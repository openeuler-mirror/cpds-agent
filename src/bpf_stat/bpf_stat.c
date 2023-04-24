/*
    eBPF 用户空间代码

	功能：
    1）加载并运行eBPF内核程序
    2）与eBPF内核程序交互，获取统计信息

    注：bpf_stat.skel.h 文件由bpftool根据eBPF内核代码自动生成
*/

#include "bpf_stat.h"
#include "bpf_stat.skel.h"
#include "logger.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static struct bpf_stat_bpf *skel = NULL;

int start_bpf_stat_monitor()
{
	int err = 0;

	skel = bpf_stat_bpf__open();
	if (skel == NULL) {
		CPDS_LOG_ERROR_PRINT("Failed to open BPF skeleton");
		return -1;
	}

	err = bpf_stat_bpf__load(skel);
	if (err) {
		CPDS_LOG_ERROR_PRINT("Failed to load and verify BPF skeleton");
		goto cleanup;
	}

	err = bpf_stat_bpf__attach(skel);
	if (err) {
		CPDS_LOG_ERROR_PRINT("Failed to attach BPF skeleton");
		goto cleanup;
	}

	CPDS_LOG_INFO("eBPF program successfully started!");

	return 0;

cleanup:
	destory_bpf_stat_monitor();
	return -err;
}

void destory_bpf_stat_monitor()
{
	if (skel != NULL) {
		bpf_stat_bpf__destroy(skel);
		skel = NULL;
	}
}

int set_perf_stat_pid_list(int pid_arr[], int pid_num)
{
	if (skel == NULL) {
		CPDS_LOG_ERROR("eBPF program NOT loaded.");
		return -1;
	}

	if (pid_arr == NULL) {
		CPDS_LOG_ERROR("Para NULL");
		return -1;
	}

	if (pid_num > MAX_STAT_MAP_SIZE) {
		CPDS_LOG_ERROR("Too many perf monitor num.");
		return -1;
	}

	// TODO: 锁

	/*
	遍历统计perf信息的bpf map，与需要监控的pid列表（数组）进行融合：
	1) 若匹配到需要监控的pid则进行标记，没有则从map中移除；
	2) 没有标记的监控pid属于新增，需要插入到bpf map中；
	*/
	int pid_exist_flags[MAX_STAT_MAP_SIZE] = {0};
	int curr_key = 0, next_key = 0;
	int bpf_ret = bpf_map__get_next_key(skel->maps.perf_stat_map, NULL, &curr_key, sizeof(int));
	while (bpf_ret == 0) {
		bpf_ret = bpf_map__get_next_key(skel->maps.perf_stat_map, &curr_key, &next_key, sizeof(int));
		int i = 0;
		for (i = 0; i < pid_num; i++) {
			if (curr_key == pid_arr[i]) {
				// 匹配到监控pid，做标记
				pid_exist_flags[i] = 1;
				break;
			}
		}
		if (i >= pid_num) {
			// 没找到匹配，从bpf map中删除
			bpf_map__delete_elem(skel->maps.perf_stat_map, &curr_key, sizeof(int), 0);
		}
		curr_key = next_key;
	}

	// 向bpf map中插入没有标记的pid
	for (int i = 0; i < pid_num; i++) {
		if (pid_exist_flags[i] != 1) {
			static perf_stat_t s = {0};
			bpf_map__update_elem(skel->maps.perf_stat_map, &pid_arr[i], sizeof(int), &s, sizeof(s), 0);
		}
	}

	return 0;
}

int get_perf_stat(int pid, perf_stat_t *stat)
{
	if (skel == NULL) {
		CPDS_LOG_ERROR("eBPF program NOT loaded");
		return -1;
	}

	if (stat == NULL) {
		CPDS_LOG_ERROR("Para NULL");
		return -1;
	}

	if (bpf_map__lookup_elem(skel->maps.perf_stat_map, &pid, sizeof(int), stat, sizeof(perf_stat_t), 0) == 0)
		return 0;

	return -1;
}

int set_process_monitor_list(monitor_process_info info_arr[], int num)
{
	struct bpf_map *pmap = skel->maps.pid_monitor_map;

	int curr_key = 0, next_key = 0;
	int bpf_ret = bpf_map__get_next_key(pmap, NULL, &curr_key, sizeof(int));
	while (bpf_ret == 0) {
		bpf_ret = bpf_map__get_next_key(pmap, &curr_key, &next_key, sizeof(int));
		bpf_map__delete_elem(pmap, &curr_key, sizeof(int), 0);
		curr_key = next_key;
	}

	for (int i = 0; i < num; i++) {
		bpf_map__update_elem(pmap, &info_arr[i].pid, sizeof(int), &info_arr[i].container_pid, sizeof(int), 0);
	}

	return 0;
}