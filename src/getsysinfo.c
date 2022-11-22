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

}

float get_sysIoWriteSize()
{

    char cmd[] = "iostat -d -x";
    char buffer[MAXBUFSIZE];
    char a[20];
    float arr[20];

    //读取执行iostat -d -x命令的结果返回给指针*fp
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

    //把iostat -d -x执行结果放入arr数组中，最后返回arr数组中的值
    sscanf(buffer, "%s %f %f %f %f %f %f %f %f %f %f %f %f %f ", a, &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5], &arr[6], &arr[7], &arr[8], &arr[9], &arr[10], &arr[11], &arr[12]);

    pclose(fp);
    return arr[12];
}
