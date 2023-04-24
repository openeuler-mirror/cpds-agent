#ifndef _BPF_STAT_TYPE_H_
#define _BPF_STAT_TYPE_H_

// bpf的map最多键值对数量
#define MAX_STAT_MAP_SIZE 200

// 容器性能统计信息
typedef struct _perf_stat {
	unsigned long total_mmap_count;
	unsigned long total_mmap_fail_count;
	unsigned long long total_mmap_size;
	unsigned long long total_mmap_time_ns;
	unsigned long total_fork_fail_cnt;
} perf_stat_t;

#endif
