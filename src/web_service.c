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

#include "web_service.h"
#include "logger.h"
#include "promhttp.h"

static struct MHD_Daemon *s_daemon = NULL;

int start_http_service(int port)
{
	if (s_daemon != NULL) {
		CPDS_LOG_ERROR("Error: The http daemon is already running!");
		return -1;
	}

	// Set the active registry for the HTTP handler
	promhttp_set_active_collector_registry(NULL);

	// start http s_daemon
	s_daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL);
	if (s_daemon == NULL) {
		CPDS_LOG_ERROR("start http daemon fail");
		return -1;
	}

	return 0;
}

void stop_http_service()
{
	if (s_daemon != NULL)
		MHD_stop_daemon(s_daemon);
}