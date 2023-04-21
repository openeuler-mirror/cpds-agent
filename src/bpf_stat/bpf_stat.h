#ifndef _BPF_STAT_H_
#define _BPF_STAT_H_

#include "bpf_stat_type.h"

typedef struct _monitor_process_info {
	int pid;
	int container_pid;
} monitor_process_info;

int start_bpf_stat_monitor();
void destory_bpf_stat_monitor();

int set_perf_stat_pid_list(int pid_arr[], int pid_num);
int get_perf_stat(int pid, perf_stat_t *stat);

int set_process_monitor_list(monitor_process_info info_arr[], int num);

#endif
