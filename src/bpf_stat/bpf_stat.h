/* 
 *  Copyright 2023 CPDS Author
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *       https://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License. 
 */

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
