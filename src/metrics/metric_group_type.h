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

#ifndef _METRIC_GROUP_TYPE_H_
#define _METRIC_GROUP_TYPE_H_

#include <glib.h>

typedef GList metric_list;

// 一个metric group包含多个同类的metrics
struct _metric_group {
	char *name;
	metric_list *metrics;
	int update_period; // 更新周期，单位s
	void (*init)();
	void (*destroy)();
	void (*update)(); // 注意：update函数内部不能长时间阻塞！
};
typedef struct _metric_group metric_group;

typedef GList metric_group_list;

#endif