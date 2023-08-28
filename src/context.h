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

#ifndef _CONTEX_H_
#define _CONTEX_H_

#include <glib.h>

#define DEFAULT_CFG_FILE "/etc/cpds/agent/config.json"
#define DEFAULT_LOG_CFG_FILE "/etc/cpds/agent/log.conf"
#define DEFAULT_EXPOSE_PORT 20001
#define DEFAULT_NET_DIAGNOSTIC_DEST "127.0.0.1"

typedef struct _agent_context {
	gboolean show_version;
	gchar *config_file;
	gchar *log_cfg_file;
	gint expose_port;
	gchar *net_diagnostic_dest;
} agent_context;

// 全局上下文
extern agent_context global_ctx;

void free_global_context();

#endif
