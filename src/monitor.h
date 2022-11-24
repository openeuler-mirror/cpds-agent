#ifndef MONITOR_H
#define MONITOR_H

#define SYS_DISK_BUFF_LEN 256
#define SYS_DISK_NAME_LEN 80
#define SYS_100_PERSENT 100
#define MAXBUFSIZE 1024
#define WAIT_SECOND 3 
typedef struct sysinfo
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