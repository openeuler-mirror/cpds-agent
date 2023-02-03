#include "../libs/zlog/zlog.h"

#define CPDS_ZLOG_INIT(PATH,CPDS_CLASS)          dzlog_init(PATH, CPDS_CLASS)
#define CPDS_ZLOG_FINI()                         zlog_fini()
#define CPDS_ZLOG_INFO(...)                      dzlog_info( __VA_ARGS__)
#define CPDS_ZLOG_DEBUG(...)                     dzlog_debug( __VA_ARGS__)
#define CPDS_ZLOG_WARN(...)                      dzlog_warn( __VA_ARGS__)
#define  CPDS_ZLOG_ERROR(...)                     dzlog_error( __VA_ARGS__)
#define CPDS_ZLOG_FATAL(...)                     dzlog_fatal( __VA_ARGS__)
