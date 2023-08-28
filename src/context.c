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

#include "context.h"

agent_context global_ctx = {
	.show_version = FALSE,
	.config_file = NULL,
	.log_cfg_file = NULL,
	.net_diagnostic_dest = NULL,
	.expose_port = 0
};

void free_global_context()
{
	agent_context *ctx = &global_ctx;
	if (ctx->config_file) {
		g_free(ctx->config_file);
		ctx->config_file = NULL;
	}
	if (ctx->log_cfg_file) {
		g_free(ctx->log_cfg_file);
		ctx->log_cfg_file = NULL;
	}
	if (ctx->net_diagnostic_dest) {
		g_free(ctx->net_diagnostic_dest);
		ctx->net_diagnostic_dest = NULL;
	}
}
