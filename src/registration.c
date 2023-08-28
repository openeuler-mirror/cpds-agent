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

#include "registration.h"
#include "collection.h"
#include "prom.h"

#include <glib.h>

int init_default_prometheus_registry()
{
	int ret = prom_collector_registry_default_init();
	return ret;
}

int register_metircs_to_default_registry(metric_group_list *mgroups)
{
	int ret = 0;

	GList *giter = NULL;
	for (giter = g_list_first(mgroups); giter != NULL; giter = g_list_next(giter)) {
		metric_group *group = (metric_group *)giter->data;
		GList *miter = NULL;
		for (miter = g_list_first(group->metrics); miter != NULL; miter = g_list_next(miter)) {
			prom_metric_t *metric = (prom_metric_t *)miter->data;
			int err = prom_collector_registry_register_metric(metric);
			if (err != 0) {
				ret = -1;
				break;
			}
		}
	}

	return ret;
}

int destroy_default_prometheus_registry()
{
	return prom_collector_registry_default_destroy();
}