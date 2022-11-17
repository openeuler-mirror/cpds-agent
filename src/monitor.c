#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "monitor.h"
struct sysinfo
{
    double CpuUsage;
    float DiskUsage;
    float IoWriteSize;
};

int main()
{
    struct sysinfo sys;
    int res;
    pthread_t mythread1, mythread2;

    res = pthread_create(&mythread1, NULL, getSysinfo, &sys);
    res = pthread_create(&mythread2, NULL, pushSysinfo, &sys);

    return 0;
}