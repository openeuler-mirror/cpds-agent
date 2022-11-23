#ifndef MONITOR_H
#define MONITOR_H

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