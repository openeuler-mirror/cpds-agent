#ifndef CPDSLOG_H
#define CPDSLOG_H

#include "../libs/zlog/zlog.h"

#define CPDS_ZLOG_INIT(CONFIG) zlog_init(CONFIG)
#define CPDS_ZLOG_GET_CATEGORY(CPDS_CLASS) zlog_get_category(CPDS_CLASS)
#define CPDS_ZLOG_INFO(...)  zlog_info( __VA_ARGS__)
#define CPDS_ZLOG_DEBUG(...) zlog_debug( __VA_ARGS__)
#define CPDS_ZLOG_WARN(...) zlog_warn( __VA_ARGS__)
#define CPDS_ZLOG_ERROR(...) zlog_error( __VA_ARGS__)
#define CPDS_ZLOG_FATAL(...) zlog_fatal( __VA_ARGS__)
