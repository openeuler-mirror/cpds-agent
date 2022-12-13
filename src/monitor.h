#ifndef MONITOR_H
#define MONITOR_H

#define RESULT_FAILED -1
#define RESULT_SUCCESS 0

#define SYS_DISK_BUFF_LEN 256
#define SYS_DISK_NAME_LEN 80
#define SYS_100_PERSENT 100
#define MAXBUFSIZE 1024
#define WAIT_SECOND 3 

#define FIELD_CPU_USAGE "CpuUsage"
#define FIELD_DISK_USAGE  "DiskUsage"
#define FIELD_IO_WRITESIZE  "IoWriteSize"

#define CONFIG_PATH "/etc/cpds/cpds-agent/cpds_log.conf"
#define CPDSCLASS_NAME "cpds-agent"
typedef struct systeminfo
{
    double CpuUsage;
    float DiskUsage;
    float IoWriteSize;
}sys_t;

void* getSysinfo(void* arg);
void* pushSysinfo(void* arg);
float get_sysCpuUsage();
double get_sysDiskUsage();
float get_sysIoWriteSize();

#endif