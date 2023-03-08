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
