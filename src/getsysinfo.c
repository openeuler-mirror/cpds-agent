#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/statfs.h>
#include "monitor.h"
#include "database.h"
#include "cpdslog.h"
#include "../libs/sqlite3/sqlite3.h"

pthread_mutex_t mut;

LinkList *creat_net_linklist()
{
    LinkList *head = (LinkList *)malloc(sizeof(LinkList));
    if (NULL == head)
    {
        CPDS_ZLOG_ERROR("malloc error!");
        return NULL;
    }
    head->stat = -1;
    head->name = NULL;
    head->next = head;
    return head;
}

//TODO:该版本这里为功能测试代码，后续版本会做一个定时类任务,重新定义方法名称
void* getSysinfo(void* arg)
{
    CPDS_ZLOG_INFO("obtain the system information about the node");

    pthread_mutex_lock(&mut); 
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
    pthread_mutex_unlock(&mut); 
}

void *pushSysinfo(void *arg)
{
    CPDS_ZLOG_INFO("upload system information to the database");
    
    pthread_mutex_lock(&mut);
    sys_t *sys = (sys_t *)arg;
    int count=5;
    while (((add_record( FIELD_CPU_USAGE, sys->CpuUsage)) < 0) || ((add_record(FIELD_DISK_USAGE, sys->DiskUsage)) < 0) || ((add_record(FIELD_IO_WRITESIZE, sys->IoWriteSize)) < 0))
    {
        if (count == 0)
        {
            break;
        }
        count--;
    }
    pthread_mutex_unlock(&mut);
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
        CPDS_ZLOG_ERROR("fopen error - '%s'", strerror(errno));
        return RESULT_FAILED;
    }

    if (fgets(buf, sizeof(buf), fp) == NULL)
    {
        CPDS_ZLOG_ERROR("fgets error - '%s'", strerror(errno));
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
        CPDS_ZLOG_ERROR("fgets error - '%s'", strerror(errno));
        fclose(fp);
        return RESULT_FAILED;
    }
    sscanf(buf, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
    all2 = user + nice + sys + idle + iowait + irq + softirq;
    idle2 = idle;

    //计算cpu使用率
    usage = (float)(all2 - all1 - (idle2 - idle1)) / (all2 - all1) * 100;

    CPDS_ZLOG_DEBUG("cpdsusage: %4.2f", usage);

    fclose(fp);
    return usage;
}

double get_sysDiskUsage()
{
    char buf[] = "/";
    struct statfs stStatfs;
    int lSts;
    double percentage; 

    memset(&stStatfs, 0, sizeof(struct statfs));
    lSts = 0;

    lSts = statfs(buf, &stStatfs);
    if (0 != lSts)
    {
        CPDS_ZLOG_ERROR("statfs faile");
        return RESULT_FAILED;
    }

    percentage = (stStatfs.f_blocks - stStatfs.f_bfree) * 100 / (stStatfs.f_blocks - stStatfs.f_bfree + stStatfs.f_bavail) + 1;
    
    CPDS_ZLOG_DEBUG("diskusage: %4.2f", percentage);

    return percentage;
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
        CPDS_ZLOG_ERROR("popen error - '%s'", strerror(errno));
        return RESULT_FAILED;
    }

    //获取执行iostat命令第四行结果
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    sscanf(buffer, "%s %f %f %f %f %f %f %f %f %f %f %f %f %f ", a, &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5], &arr[6], &arr[7], &arr[8], &arr[9], &arr[10], &arr[11], &arr[12]);

    CPDS_ZLOG_DEBUG(" wareq-sz : %f", arr[12]);

    pclose(fp);
    return arr[12];
}

//TODO:该版本这里为功能测试代码，后须会完善成多网卡
int get_netlink_status(const char *if_name)
{
    int skfd;
    struct ifreq ifr;
    struct ethtool_value netcard;

    netcard.cmd = ETHTOOL_GLINK;
    netcard.data = 0; 
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *)&netcard;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0)
    {
        CPDS_ZLOG_ERROR("create socket fail");
        return RESULT_FAILED;
    }

    if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1)
    {
        CPDS_ZLOG_ERROR("failed to obtain the NIC status");
        close(skfd);
        return RESULT_FAILED;
    }

    close(skfd);
    return netcard.data;
}

//TODO:该版本这里为获取多张网卡名功能测试代码，后须会完善获取状态并存入链表中
int get_multiple_netlink()
{
    const char *stat;
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in6 *sa;
    struct sockaddr_in *sa4;
    char addr[INET6_ADDRSTRLEN];
    char addr4[INET_ADDRSTRLEN];

    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            sa = (struct sockaddr_in6 *)ifa->ifa_addr;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr,
                        sizeof(addr), NULL, 0, NI_NUMERICHOST);

            CPDS_ZLOG_DEBUG("NIC NAME: %s ", ifa->ifa_name);
            stat = get_netlink_status(ifa->ifa_name) == 1 ? "up" : "down";
            CPDS_ZLOG_DEBUG("Net link status: %s", stat);
        }
        else 
        {
            sa4 = (struct sockaddr_in *)ifa->ifa_addr;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), addr4,
                        sizeof(addr4), NULL, 0, NI_NUMERICHOST);

            CPDS_ZLOG_DEBUG("NIC NAME: %s ", ifa->ifa_name);
            stat = get_netlink_status(ifa->ifa_name) == 1 ? "up" : "down";
            CPDS_ZLOG_DEBUG("Net link status: %s", stat);
        }
    }

    freeifaddrs(ifap);
    return RESULT_SUCCESS;
}