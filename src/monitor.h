#ifndef MONITOR_H
#define MONITOR_H

void* getSysinfo(void* arg);
void* pushSysinfo(void* arg);
int get_sysCpuUsage();
int get_sysDiskUsage();
float get_sysIoWriteSize();

#endif