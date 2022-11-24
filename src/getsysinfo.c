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

float get_sysCpuUsage()
{
    FILE *fp;
    char buf[128];
    char cpu[5];
    long int user, nice, sys, idle, iowait, irq, softirq;
    long int all1, all2, idle1, idle2;
    float usage;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("fopen error");
        return -1;
    }

    if (fgets(buf, sizeof(buf), fp) == NULL)
    {
        perror("fgets");
        fclose(fp);
        return -1;
    }

    //cpu名称、用户态时间、nice用户态时间、内核态时间、空闲时间、I/O等待时间、硬中断时间、软中断时间
    sscanf(buf, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
    all1 = user + nice + sys + idle + iowait + irq + softirq;
    idle1 = idle;

    rewind(fp);
    sleep(1);
    memset(buf, 0, sizeof(buf));
    cpu[0] = '\0';
    user = nice = sys = idle = iowait = irq = softirq = 0;

    if (fgets(buf, sizeof(buf), fp) == NULL)
    {
        perror("fgets");
        fclose(fp);
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

double get_sysDiskUsage()
{
    FILE *fp;
    char filesystem[SYS_DISK_NAME_LEN], available[SYS_DISK_NAME_LEN], use[SYS_DISK_NAME_LEN], mounted[SYS_DISK_NAME_LEN], buf[SYS_DISK_BUFF_LEN];
    double used, blocks, used_rate;

    fp = popen("df", "r");
    if (fp == NULL)
    {
        perror("popen error");
        return -1;
    }

    fgets(buf, SYS_DISK_BUFF_LEN, fp);
    double dev_total = 0, dev_used = 0;

    //累加每个设备大小之和赋值给dev_total,累加每个设备已用空间之和赋值给dev_used
    while (6 == fscanf(fp, "%s %lf %lf %s %s %s", filesystem, &blocks, &used, available, use, mounted))
    {
        dev_total += blocks;
        dev_used += used;
    }

    used_rate = (dev_used / dev_total) * SYS_100_PERSENT;

    pclose(fp);
    return used_rate;
}

float get_sysIoWriteSize()
{

    char cmd[] = "iostat -d -x";
    char buffer[MAXBUFSIZE];
    char a[20];
    float arr[20];

    FILE *fp = popen(cmd, "r");
    if (!fp)
    {
        perror("popen error");
        return -1;
    }

    //获取执行iostat命令第四行结果
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    sscanf(buffer, "%s %f %f %f %f %f %f %f %f %f %f %f %f %f ", a, &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5], &arr[6], &arr[7], &arr[8], &arr[9], &arr[10], &arr[11], &arr[12]);

    pclose(fp);
    return arr[12];
}
