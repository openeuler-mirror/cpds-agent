#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "monitor.h"
#define SYS_DISK_BUFF_LEN 256
#define SYS_DISK_NAME_LEN 80
#define SYS_100_PERSENT 100
#define MAXBUFSIZE 1024
#define WAIT_SECOND 3 

void* getSysinfo(void* arg)
{
    sys_t *sys = (sys_t *)arg;
}

void* pushSysinfo(void* arg)
{
    sys_t *sys = (sys_t *)arg;
}

int get_sysCpuUsage()
{
    FILE *fp;
    char buf[128];
    char cpu[5];
    long int user, nice, sys, idle, iowait, irq, softirq;
    long int all1, all2, idle1, idle2;
    float usage;

    //读取/proc/stat内容，返回给指针fp
    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("fopen error");
        return -1;
    }

    if (fgets(buf, sizeof(buf), fp) == NULL)
    {
        perror("fgets");
        return -1;
    }

    //将cpu指标之和赋值给all1：用户态时间+nice用户态时间+内核态时间+空闲时间+I/O等待时间+硬中断时间+软中断时间
    sscanf(buf, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
    all1 = user + nice + sys + idle + iowait + irq + softirq;
    idle1 = idle;

    //将文件指针、buf数组及相关变量还原为初始值
    rewind(fp);
    sleep(1);
    memset(buf, 0, sizeof(buf));
    cpu[0] = '\0';
    user = nice = sys = idle = iowait = irq = softirq = 0;

    //重新获取fp中内容，重新将buf中值赋值给定义变量，将cpu指标和空间时间分别赋值给all2和idle2
    if (fgets(buf, sizeof(buf), fp) == NULL)
    {
        perror("fgets");
        return -1;
    }
    sscanf(buf, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
    all2 = user + nice + sys + idle + iowait + irq + softirq;
    idle2 = idle;

    //计算cpu使用率
    usage = (float)(all2 - all1 - (idle2 - idle1)) / (all2 - all1) * 100;

    fclose(fp);
    return usage;
}

int get_sysDiskUsage()
{

}

float get_sysIoWriteSize()
{

}
