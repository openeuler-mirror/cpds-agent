/*
    eBPF 内核空间代码

    功能：
    1）利用内核tracepoint等技术挂接hook，提取统计信息
    2）使用bpf maps技术与用户空间交换数据

    两个核心表（bpf maps）：
    1）perf_stat_map：key是容器的主pid，bpf程序会将容器中的所有进程的性能统计信息汇总后填入
    2）pid_monitor_map：key是容器内部所有pid，bpf程序依据这些pid对执行过程的特定内核函数调用进行数据抓取

    注：编译依赖libbpf
*/

#include "bpf_stat_type.h"
#include "vmlinux.h"
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>


char LICENSE[] SEC("license") = "Dual BSD/GPL";

typedef struct _sys_enter_mmap_stat {
	int container_pid;        // 容器pid
	unsigned long alloc_size; // 分配内存大小
	unsigned long time_start; // 进入mmap调用的时间（单位ns）
} sys_enter_mmap_stat_t;

// 记录一个线程调用sys_enter_mmap信息，仅bpf内核程序内部计算使用
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_STAT_MAP_SIZE);
	__type(key, int); //tid
	__type(value, sys_enter_mmap_stat_t);
} sys_enter_mmap_stat_map SEC(".maps");

// 容器及其子进程监控列表
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_STAT_MAP_SIZE);
	__type(key, int); //pid
	__type(value, int); //container pid
} pid_monitor_map SEC(".maps");

// 记录一个容器所有进程总体性能统计信息，用户空间可读取
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_STAT_MAP_SIZE);
	__type(key, int); //container pid
	__type(value, perf_stat_t);
} perf_stat_map SEC(".maps");

typedef struct _sys_enter_clone_stat {
	int container_pid; // 容器pid
	int is_thread;     // 0: 创建进程； 1: 创建线程
} sys_enter_clone_t;

// 记录容器创建进程/线程信息，仅bpf内核程序内部计算使用
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_STAT_MAP_SIZE);
	__type(key, int); //tid
	__type(value, sys_enter_clone_t);
} sys_enter_clone_map SEC(".maps");

struct sys_enter_mmap_para {
	__u64 pad;
	int __syscall_nr;
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long off;
};

SEC("tracepoint/syscalls/sys_enter_mmap")
int handle_sys_enter_mmap(struct sys_enter_mmap_para *ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	int pid = id >> 32;
	int tid = (int)id;

	// 内核内部计算的map以线程id为key
	int *p = bpf_map_lookup_elem(&pid_monitor_map, &pid);
	if (p) {
		sys_enter_mmap_stat_t semst = {0};
		semst.container_pid = *p;
		semst.time_start = bpf_ktime_get_ns();
		semst.alloc_size = ctx->len;
		bpf_map_update_elem(&sys_enter_mmap_stat_map, &tid, &semst, BPF_ANY);
		// bpf_printk(">>> [%d][%d] bpf_sys_enter_mmap:", pid, tid);
		// bpf_printk(">>>    cpid=%d, size=%lu\n", semst.container_pid, ctx->len);
	}

	return 0;
}

struct exit_mmap_t {
	__u64 pad;
	int __syscall_nr;
	long ret;
};

SEC("tracepoint/syscalls/sys_exit_mmap")
int handle_exit_mmap(struct exit_mmap_t *ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	int pid = id >> 32;
	int tid = (int)id;

	sys_enter_mmap_stat_t *semst = bpf_map_lookup_elem(&sys_enter_mmap_stat_map, &tid);
	if (!semst)
		return 0;

	perf_stat_t *s = bpf_map_lookup_elem(&perf_stat_map, &semst->container_pid);
	if (s) {
		s->total_mmap_time_ns += bpf_ktime_get_ns() - semst->time_start;
		s->total_mmap_count += 1;
		if (ctx->ret <= 0)
			s->total_mmap_fail_count += 1;
		s->total_mmap_size += semst->alloc_size;
		// bpf_printk(">>> [%d][%d] bpf_sys_exit_mmap: ret=%ld\n", pid, tid, ctx->ret);
		// bpf_printk("    cpid=%d, total_mmap_size=%lu\n", semst->container_pid, s->total_mmap_size);
	}

	bpf_map_delete_elem(&sys_enter_mmap_stat_map, &tid);

	return 0;
}

struct enter_clone_t {
	__u64 pad;
	int __syscall_nr;
	unsigned long clone_flags;
	unsigned long newsp;
	unsigned long parent_tidptr;
	unsigned long tls;
	unsigned long child_tidptr;
};

#ifndef CLONE_THREAD
#define CLONE_THREAD 0x00010000
#endif

SEC("tracepoint/syscalls/sys_enter_clone")
int handle_enter_clone(struct enter_clone_t *ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	int pid = id >> 32;
	int tid = (int)id;

	int *cpid = bpf_map_lookup_elem(&pid_monitor_map, &pid);
	if (cpid) {
		sys_enter_clone_t st = {0};
		st.container_pid = *cpid;
		if (ctx->clone_flags & CLONE_THREAD) {
			// bpf_printk("tttttt [%d][%d] clone thread", pid, tid);
			st.is_thread = 1;
		} else {
			// bpf_printk("pppppp [%d][%d] clone process", pid, tid);
			st.is_thread = 0;
		}
		bpf_map_update_elem(&sys_enter_clone_map, &tid, &st, BPF_ANY);
	}

	return 0;
}

struct exit_clone_t {
	__u64 pad;
	int __syscall_nr;
	long ret;
};

SEC("tracepoint/syscalls/sys_exit_clone")
int handle_exit_clone(struct exit_clone_t *ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	int pid = id >> 32;
	int tid = (int)id;

	sys_enter_clone_t *st = bpf_map_lookup_elem(&sys_enter_clone_map, &tid);
	if (!st)
		return 0;

	// 记录 进程/线程 创建失败
	if (ctx->ret < 0) {
		perf_stat_t *s = bpf_map_lookup_elem(&perf_stat_map, &st->container_pid);
		if (s) {
			if (st->is_thread == 0) {
				s->total_create_process_fail_cnt += 1;
				// bpf_printk(">>> [%d][%d] create process fail: ret=%ld", pid, tid, ctx->ret);
			} else {
				s->total_create_thread_fail_cnt += 1;
				// bpf_printk(">>> [%d][%d] create thread fail: ret=%ld", pid, tid, ctx->ret);
			}
		}
	}

	bpf_map_delete_elem(&sys_enter_clone_map, &tid);

	return 0;
}

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	int pid = id >> 32;
	int tid = (int)id;

	struct task_struct *task = (struct task_struct *)bpf_get_current_task();
	if (!task)
		return 0;
	int ppid = BPF_CORE_READ(task, real_parent, tgid);
	int *cpid = bpf_map_lookup_elem(&pid_monitor_map, &ppid);
	if (cpid) {
		// 容器子进程添加监控
		bpf_map_update_elem(&pid_monitor_map, &pid, cpid, BPF_ANY);
		// bpf_printk(">>> [%d][%d] container sched_process_exec: ", pid, tid);
		// bpf_printk("    ppid=%d, cpid=%d\n", ppid, *cpid);
	}

	return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_template* ctx)
{
	__u64 id = bpf_get_current_pid_tgid();
	int pid = id >> 32;
	int tid = (int)id;

	int *cpid = bpf_map_lookup_elem(&pid_monitor_map, &pid);
	if (cpid) {
		// 容器子进程移除监控
		bpf_map_delete_elem(&pid_monitor_map, &pid);
	}
		
	return 0;
}
