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

#include "container_collector.h"
#include "metric_group_type.h"
#include "prom.h"

static void group_container_process_init();
static void group_container_process_destroy();
static void group_container_process_update();

metric_group group_container_process = {.name = "container_process_group",
                                        .update_period = 3,
                                        .init = group_container_process_init,
                                        .destroy = group_container_process_destroy,
                                        .update = group_container_process_update};

static prom_gauge_t *cpds_container_sub_process_info;

static void group_container_process_init()
{
	metric_group *grp = &group_container_process;
	const char *labels[] = {"container", "pid", "zombie"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_container_sub_process_info = prom_gauge_new("cpds_container_sub_process_info", "container sub process infomation", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_sub_process_info);
}

static void group_container_process_destroy()
{
	if (group_container_process.metrics)
		g_list_free(group_container_process.metrics);
}

static void update_container_process_info(GList *plist)
{
	prom_gauge_clear(cpds_container_sub_process_info);

	GList *iter = plist;
	while (iter != NULL) {
		ctn_process_metric *cpm = iter->data;
		GList *sub_iter = cpm->ctn_sub_process_stat_list;
		while (sub_iter != NULL) {
			ctn_sub_process_stat_metric *cspsm = sub_iter->data;
			char str_pid[10] = {0};
			g_snprintf(str_pid, sizeof(str_pid), "%d", cspsm->pid);
			const char *str_zombie = cspsm->zombie_flag == 1 ? "true" : "false";
			prom_gauge_set(cpds_container_sub_process_info, 1, (const char *[]){cpm->cid, str_pid, str_zombie});
			sub_iter = sub_iter->next;
		}
		iter = iter->next;
	}
}

static void group_container_process_update()
{
	get_ctn_process_metric(update_container_process_info);
}
