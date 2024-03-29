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

#include "metric_groups.h"

extern metric_group group_agent_alive;
extern metric_group group_node_basic;
extern metric_group group_node_network;
extern metric_group group_node_cpu;
extern metric_group group_node_memory;
extern metric_group group_node_fs;
extern metric_group group_node_disk;
extern metric_group group_node_service;
extern metric_group group_container_service;
extern metric_group group_container_basic;
extern metric_group group_container_resource;
extern metric_group group_container_perf;
extern metric_group group_container_process;
extern metric_group group_kernel_fault;
extern metric_group group_pod_info;

metric_group_list *init_metric_groups(metric_group_list *mgroups)
{
	mgroups = g_list_append(mgroups, &group_agent_alive);
	mgroups = g_list_append(mgroups, &group_node_basic);
	mgroups = g_list_append(mgroups, &group_node_network);
	mgroups = g_list_append(mgroups, &group_node_cpu);
	mgroups = g_list_append(mgroups, &group_node_memory);
	mgroups = g_list_append(mgroups, &group_node_fs);
	mgroups = g_list_append(mgroups, &group_node_disk);
	mgroups = g_list_append(mgroups, &group_node_service);
	mgroups = g_list_append(mgroups, &group_container_service);
	mgroups = g_list_append(mgroups, &group_container_resource);
	mgroups = g_list_append(mgroups, &group_container_basic);
	mgroups = g_list_append(mgroups, &group_container_perf);
	mgroups = g_list_append(mgroups, &group_container_process);
	mgroups = g_list_append(mgroups, &group_kernel_fault);
	mgroups = g_list_append(mgroups, &group_pod_info);
	return mgroups;
}
