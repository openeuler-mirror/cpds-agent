#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "monitor.h"
#include "database.h"
#include "../util/sqlite3/sqlite3.h"

int create_sysinfotable(sqlite3 *db)
{
    char *errmsg = NULL;

    if (SQLITE_OK != sqlite3_exec(db, CREATE_SYSINFOTABLE, NULL, NULL, &errmsg))
    {
        printf("create table error!%s\n", errmsg);
        sqlite3_free(errmsg);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

int add_record(sqlite3 *db, char *name, float data)
{
    char sql[128];
    char *errmsg = NULL;

    sprintf(sql, INSERT_INTO_SYSINFOTABLE, name, data);
    if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        printf("add record fail!%s\n", errmsg);
        sqlite3_free(errmsg);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}
//TODO:该版本这里为功能测试代码输入删除信息，后续会做成接口获取删除信息
int delete_record(sqlite3 *db, int id)
{
    char sql[128];
    char *errmsg = NULL;

    sprintf(sql, DELETE_IN_TABLE_SYSINFO, id);
    if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        printf("delete error! %s\n", errmsg);
        sqlite3_free(errmsg);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}
//TODO:该版本这里为功能测试代码,实现回调函数打印表信息的方法，后续会将信息打印到日志里
int display(void *para, int ncol, char *col_val[], char **col_name)
{
    int *flag = NULL;
    int i;
    flag = (int *)para;

    if (0 == *flag)
    {
        *flag = 1;
        printf("column number is:%d\n", ncol); //flag=0为第一次，打印列的名称
        for (i = 0; i < ncol; i++)
        {
            printf("%10s", col_name[i]);
        }
        printf("\n");
    }
    for (i = 0; i < ncol; i++) //打印值
    {
        printf("%10s", col_val[i]);
    }
    printf("\n");
    return 0;
}
//TODO:该版本这里为功能测试代码，后续版本会做一个定时类任务,重新定义方法名称
void* getSysinfo(void* arg)
{
    sys_t *sys = (sys_t *)arg;
    int count = 5;
    while (1)
    {
        sleep(1);
        while (((sys->CpuUsage = get_sysCpuUsage()) < 0) || ((sys->DiskUsage = get_sysDiskUsage()) < 0) || ((sys->IoWriteSize = get_sysIoWriteSize() < 0)))
        {
            if (count == 0)
            {
                break;
            }
            count--;
        }
    }
}

void *pushSysinfo(void *arg)
{
    sys_t *sys = (sys_t *)arg;
    while (((add_record(db, "CpuUsage", sys->CpuUsage)) < 0) || ((add_record(db, "DiskUsage", sys->DiskUsage)) < 0) || ((add_record(db, "IoWriteSize", sys->IoWriteSize)) < 0))
    {
        if (count == 0)
        {
            break;
        }
        count--;
    }
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
        return RESULT_FAILED;
    }

    if (fgets(buf, sizeof(buf), fp) == NULL)
    {
        perror("fgets");
        fclose(fp);
        return RESULT_FAILED;
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
        return RESULT_FAILED;
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
        return RESULT_FAILED;
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
        return RESULT_FAILED;
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
