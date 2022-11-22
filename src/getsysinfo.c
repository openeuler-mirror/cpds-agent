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

}

int get_sysDiskUsage()
{
    FILE *fp;
    char filesystem[SYS_DISK_NAME_LEN], available[SYS_DISK_NAME_LEN], use[SYS_DISK_NAME_LEN], mounted[SYS_DISK_NAME_LEN], buf[SYS_DISK_BUFF_LEN];
    double used, blocks, used_rate;

    //读取df命令输出结果返回给指针fp
    fp = popen("df", "r");
    if (fp = NULL)
    {
        perror("popen error");
        return -1;
    }

    //将fp指针指向下一行
    fgets(buf, SYS_DISK_BUFF_LEN, fp);
    double dev_total = 0, dev_used = 0;

    //累加每个设备大小之和赋值给dev_total,累加每个设备已用空间之和赋值给dev_used
    while (6 == fscanf(fp, "%s %lf %lf %s %s %s", filesystem, &blocks, &used, available, use, mounted))
    {
        dev_total += blocks;
        dev_used += used;
    }

    //计算磁盘使用率
    used_rate = (dev_used / dev_total) * SYS_100_PERSENT;

    pclose(fp);
    return used_rate;
}

float get_sysIoWriteSize()
{

}
