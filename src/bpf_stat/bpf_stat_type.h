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
	unsigned long total_create_process_fail_cnt;
	unsigned long total_create_thread_fail_cnt;
} perf_stat_t;

#endif
