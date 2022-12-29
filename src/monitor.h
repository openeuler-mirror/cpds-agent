#ifndef MONITOR_H
#define MONITOR_H

#define RESULT_FAILED -1
#define RESULT_SUCCESS 0

#define SYS_DISK_BUFF_LEN   256
#define SYS_DISK_NAME_LEN   80
#define SYS_100_PERSENT     100
#define MAXBUFSIZE          1024
#define WAIT_SECOND         3 

#define FIELD_CPU_USAGE      "CpuUsage"
#define FIELD_DISK_USAGE     "DiskUsage"
#define FIELD_IO_WRITESIZE   "IoWriteSize"

#define CONFIG_PATH          "/etc/cpds/cpds-agent/cpds_log.conf"
#define CPDSCLASS_NAME       "cpds-agent"
typedef struct systeminfo
{
    double CpuUsage;
    float DiskUsage;
    float IoWriteSize;
}sys_t;

typedef int  data_t;
typedef char str_t;
typedef struct LNode
{
    str_t   *name;
    data_t    stat;
}NetData;

void* get_sysinfo(void* arg);
void* push_sysinfo(void* arg);
float get_syscpu_usage();
double get_sysdisk_usage();
float get_sysio_wbs();

#endif