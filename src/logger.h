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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "zlog.h"
#include <errno.h>
#include <string.h>

// 记录到文件
#define CPDS_LOG_DEBUG(fmt, ...) dzlog_debug(fmt, ##__VA_ARGS__)
#define CPDS_LOG_INFO(fmt, ...) dzlog_info(fmt, ##__VA_ARGS__)
#define CPDS_LOG_WARN(fmt, ...) dzlog_warn(fmt, ##__VA_ARGS__)
#define CPDS_LOG_ERROR(fmt, ...) dzlog_error(fmt, ##__VA_ARGS__)

//仅标准输出
#define CPDS_PRINT(fmt, ...) printf(fmt"\n", ##__VA_ARGS__)

//记录日志文件同时输出到标准输出
#define CPDS_LOG_DEBUG_PRINT(fmt, ...) CPDS_LOG_DEBUG(fmt, ##__VA_ARGS__); CPDS_PRINT(fmt, ##__VA_ARGS__)
#define CPDS_LOG_INFO_PRINT(fmt, ...) CPDS_LOG_INFO(fmt, ##__VA_ARGS__); CPDS_PRINT(fmt, ##__VA_ARGS__)
#define CPDS_LOG_WARN_PRINT(fmt, ...) CPDS_LOG_WARN(fmt, ##__VA_ARGS__); CPDS_PRINT(fmt, ##__VA_ARGS__)
#define CPDS_LOG_ERROR_PRINT(fmt, ...) CPDS_LOG_ERROR(fmt, ##__VA_ARGS__); CPDS_PRINT(fmt, ##__VA_ARGS__)

int log_init(const char *cfg);
void log_fini();

#endif
