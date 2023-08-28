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

#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

static void group_node_memory_init();
static void group_node_memory_destroy();
static void group_node_memory_update();

metric_group group_node_memory = {.name = "node_memory_group",
                                  .update_period = 2,
                                  .init = group_node_memory_init,
                                  .destroy = group_node_memory_destroy,
                                  .update = group_node_memory_update};

static prom_gauge_t *cpds_node_memory_total_bytes;
static prom_gauge_t *cpds_node_memory_free_bytes;
static prom_gauge_t *cpds_node_memory_usage_bytes;
static prom_gauge_t *cpds_node_memory_buff_cache_bytes;
static prom_gauge_t *cpds_node_memory_swap_total_bytes;
static prom_gauge_t *cpds_node_memory_swap_usage_bytes;

static void group_node_memory_init()
{
	metric_group *grp = &group_node_memory;
	cpds_node_memory_total_bytes = prom_gauge_new("cpds_node_memory_total_bytes", "node total memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_total_bytes);
	cpds_node_memory_free_bytes = prom_gauge_new("cpds_node_memory_free_bytes", "node free memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_free_bytes);
	cpds_node_memory_usage_bytes = prom_gauge_new("cpds_node_memory_usage_bytes", "node used memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_usage_bytes);
	cpds_node_memory_buff_cache_bytes = prom_gauge_new("cpds_node_memory_buff_cache_bytes", "node buff/cache memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_buff_cache_bytes);
	cpds_node_memory_swap_total_bytes = prom_gauge_new("cpds_node_memory_swap_total_bytes", "node swap total memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_swap_total_bytes);
	cpds_node_memory_swap_usage_bytes = prom_gauge_new("cpds_node_memory_swap_usage_bytes", "node swap used memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_swap_usage_bytes);
}

static void group_node_memory_destroy()
{
	if (group_node_memory.metrics)
		g_list_free(group_node_memory.metrics);
}

static void group_node_memory_update()
{
	unsigned long mem_total = 0;
	unsigned long mem_free = 0;
	unsigned long mem_buffers = 0;
	unsigned long mem_cached = 0;
	unsigned long mem_sreclaimable = 0;
	unsigned long mem_swap_total = 0;
	unsigned long mem_swap_free = 0;

	char *meminfo_content = NULL;
	if (g_file_get_contents("/proc/meminfo", &meminfo_content, NULL, NULL) == FALSE)
		return;

	char **line_arr = g_strsplit(meminfo_content, "\n", -1);
	g_free(meminfo_content);
	if (line_arr == NULL)
		return;

	int i = 0;
	while (line_arr[i] != NULL) {
		if (g_ascii_strncasecmp(line_arr[i], "MemTotal", strlen("MemTotal")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_total);
		} else if (g_ascii_strncasecmp(line_arr[i], "MemFree", strlen("MemFree")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_free);
		} else if (g_ascii_strncasecmp(line_arr[i], "Buffers", strlen("Buffers")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_buffers);
		} else if (g_ascii_strncasecmp(line_arr[i], "Cached", strlen("Cached")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_cached);
		} else if (g_ascii_strncasecmp(line_arr[i], "SReclaimable", strlen("SReclaimable")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_sreclaimable);
		} else if (g_ascii_strncasecmp(line_arr[i], "SwapTotal", strlen("SwapTotal")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_swap_total);
		} else if (g_ascii_strncasecmp(line_arr[i], "SwapFree", strlen("SwapFree")) == 0) {
			sscanf(line_arr[i], "%*s %lu", &mem_swap_free);
		}
		i++;
	}

	unsigned long buff_cache_sum = mem_buffers + mem_cached + mem_sreclaimable;
	prom_gauge_set(cpds_node_memory_total_bytes, (double)mem_total * 1024, NULL);
	prom_gauge_set(cpds_node_memory_free_bytes, (double)mem_free * 1024, NULL);
	prom_gauge_set(cpds_node_memory_buff_cache_bytes, (double)buff_cache_sum * 1024, NULL);
	prom_gauge_set(cpds_node_memory_usage_bytes, (double)(mem_total - mem_free - buff_cache_sum) * 1024, NULL);
	prom_gauge_set(cpds_node_memory_swap_total_bytes, (double)mem_swap_total * 1024, NULL);
	prom_gauge_set(cpds_node_memory_swap_usage_bytes, (double)(mem_swap_total - mem_swap_free) * 1024, NULL);

	g_strfreev(line_arr);
}
