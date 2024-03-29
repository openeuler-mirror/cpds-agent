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

#ifndef _CONTAINER_COLLECTOR_H_
#define _CONTAINER_COLLECTOR_H_

#include <glib.h>

typedef struct _ctn_basic_metric {
	char *cid; // 容器id
	int pid;
	char *status;
	int exit_code;
	char *ip_addr;
} ctn_basic_metric;

typedef struct _ctn_perf_stat_metric {
	char *cid; // 容器id
	double total_mmap_count;
	double total_mmap_fail_count;
	double total_mmap_size;
	double total_mmap_time_seconds;
	double total_create_process_fail_cnt;
	double total_create_thread_fail_cnt;
} ctn_perf_metric;

typedef struct _ctn_net_dev_stat_metric {
	char *ifname;
	double network_receive_bytes_total;
	double network_receive_drop_total;
	double network_receive_errors_total;
	double network_receive_packets_total;
	double network_transmit_bytes_total;
	double network_transmit_drop_total;
	double network_transmit_errors_total;
	double network_transmit_packets_total;
} ctn_net_dev_stat_metric;

typedef struct _ctn_net_snmp_stat_metrics {
	double network_icmp_out_type8_total;
	double network_icmp_in_type0_total;
} ctn_net_snmp_stat_metrics;

typedef struct _ctn_ping_stat_metrics {
	double send_cnt; // 发送次数
	double recv_cnt; // 接收次数
	double rtt;      // 往返时间(s)
} ctn_ping_stat_metrics;

typedef struct _ctn_resource_metrics {
	char *cid; // 容器id
	int pid;
	double disk_usage_bytes;
	double disk_iodelay;
	double cpu_usage_seconds;
	double memory_total_bytes;
	double memory_usage_bytes;
	double memory_swap_total_bytes;
	double memory_swap_usage_bytes;
	double memory_cached_bytes;
	char *network_mode;
	char *ip_addr;
	ctn_ping_stat_metrics ctn_ping_stat;
	ctn_net_snmp_stat_metrics ctn_net_snmp_stat;
	GList *ctn_net_dev_stat_list; // list of ctn_net_dev_stat_metric
} ctn_resource_metric;

typedef struct _ctn_sub_process_stat {
	int pid;
	int zombie_flag;
} ctn_sub_process_stat_metric;

typedef struct _ctn_sub_process_metrics {
	char *cid; // 容器id
	GList *ctn_sub_process_stat_list; // list of ctn_sub_process_stat_metric
} ctn_process_metric;

int start_updating_container_info();
int stop_updating_container_info();

typedef void (*PROC_CONTAINER_INFO_LIST)(GList *plist);

// 获取 ctn_basic_metric
void get_ctn_basic_metric(PROC_CONTAINER_INFO_LIST proc);

// 获取 ctn_perf_metric
void get_ctn_perf_metric(PROC_CONTAINER_INFO_LIST proc);

// 获取 ctn_resource_metric
void get_ctn_resource_metric(PROC_CONTAINER_INFO_LIST proc);

// 获取 ctn_process_metric
void get_ctn_process_metric(PROC_CONTAINER_INFO_LIST proc);

#endif
