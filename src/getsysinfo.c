#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <glib.h>
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

//TODO:该版本这里为功能测试代码，后续版本会做一个定时类任务,重新定义方法名称
void *get_sysinfo(void *arg)
{
    CPDS_ZLOG_INFO("obtain the system information about the node");
    pthread_mutex_lock(&mut); 
    sys_t *sys = (sys_t *)arg;
    int count = 5;
    while (1){
        sleep(1);
        while (((sys->CpuUsage = get_syscpu_usage()) < 0) || ((sys->DiskUsage = get_sysdisk_usage()) < 0) || ((sys->iowritesize = get_sysio_wbs() < 0))){
            if (count == 0){
                break;
            }
            count--;
        }
    }
    pthread_mutex_unlock(&mut); 
}

void *push_sysinfo(void *arg)
{
    CPDS_ZLOG_INFO("upload system information to the database");
    pthread_mutex_lock(&mut);
    sys_t *sys = (sys_t *)arg;
    int count=5;
    while (((add_record( FIELD_CPU_USAGE, sys->cpuusage)) < 0) || ((add_record(FIELD_DISK_USAGE, sys->DiskUsage)) < 0) || ((add_record(FIELD_IO_WRITESIZE, sys->IoWriteSize)) < 0)){
        if (count == 0){
            break;
        }
        count--;
    }
    pthread_mutex_unlock(&mut);
}

double calc_cpu_occupy (_cpu_info *start, _cpu_info *end)
{
    double start_total, end_total, e_idle, s_idle, cpu_use;
    //用户态时间、nice用户态时间、内核态时间、空闲时间、I/O等待时间、硬中断时间、软中断时间
    start_total = (double) (start->user + start->nice + start->system + start->idle + start->softirq + start->iowait + start->irq);
    end_total = (double) (end->user + end->nice + end->system + end->idle + end->softirq + end->iowait + end->irq);
    e_idle = (double) (end->idle);    
    s_idle = (double) (start->idle);    
    if ((end_total-start_total) != 0)
        cpu_use = 100.0 - ((e_idle-s_idle))/(end_total-start_total)*100.00;
    else
        cpu_use = 0;

    return cpu_use;
}

int get_cpu_data (_cpu_info *data)
{
    FILE *fp;
    char buf[256];
    int nscan = 0, val = RESULT_FAILED;

    fp = fopen ("/proc/stat", "r");
    if (NULL == fp) {
        CPDS_ZLOG_ERROR("fopen error - '%s'", strerror(errno));
        goto open_err;
    }

    if (fgets(buf, sizeof(buf), fp) == NULL) {
        CPDS_ZLOG_ERROR("fgets error - '%s'", strerror(errno));
        goto out;
    }
  
    nscan = sscanf(buf, "%s %u %u %u %u %u %u %u", data->name, &data->user, &data->nice, &data->system, &data->idle, &data->iowait, &data->irq, &data->softirq);
    if (nscan != 8)
    {
        if (nscan == -1)
            CPDS_ZLOG_ERROR("nscan error - '%s'", strerror(errno));
        else
            CPDS_ZLOG_ERROR("field conversion failure");
        goto out;
    }

    val = RESULT_SUCCESS;
out:
    fclose(fp);
open_err:
    return val;
    
}

int get_cpu_usage(double *cpu_usage)
{
    _cpu_info cpu_start_stat;
    _cpu_info cpu_end_stat;
    int ret, val = RESULT_FAILED;

    ret = get_cpu_data(&cpu_start_stat);
    if (ret < 0) {
        CPDS_ZLOG_ERROR("failed to obtain the start status");
        goto out;
    }
    sleep(1);
    ret = get_cpu_data(&cpu_end_stat);
    if (ret < 0) {
        CPDS_ZLOG_ERROR("failed to obtain the end status");
        goto out;
    }

    *cpu_usage = calc_cpu_occupy(&cpu_start_stat, &cpu_end_stat);

    val = RESULT_SUCCESS;
out:
    return val;

}

int get_disk_usage(float *disk_usage)
{
    char buf[] = "/";
    struct statfs st_statfs;
    int lsts;

    memset(&st_statfs, 0, sizeof(struct statfs));
    lsts = 0;

    lsts = statfs(buf, &st_statfs);
    if (0 != lsts) {
        CPDS_ZLOG_ERROR("statfs fail - '%s'", strerror(errno));
        return RESULT_FAILED;
    }

    *disk_usage = (st_statfs.f_blocks - st_statfs.f_bfree) * 100 / (st_statfs.f_blocks - st_statfs.f_bfree + st_statfs.f_bavail) + 1;
    
    CPDS_ZLOG_DEBUG("diskusage: %4.2f", *disk_usage);

    return RESULT_SUCCESS;
}

float get_syscpu_usage()
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

double get_sysdisk_usage()
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

float get_sysio_wbs()
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

void free_netlist(GList *plist)
{
    GList *ls = g_list_first(plist);
    while (ls != NULL)
    {
        NetData *release = (NetData *)ls->data;
        free(release);
        ls = g_list_next(ls);
    }
    g_list_free(plist);
    plist = NULL;
}

//TODO:该版本这里为测试打印数据成功加入链表的代码，后续会直接往数据库中写入
void output_list(GList *plist)
{
    int len;
    len = g_list_length(plist);
    if(len == 0)
    {
       CPDS_ZLOG_ERROR("empty table");
       return;
    }
    CPDS_ZLOG_DEBUG("lenth=%d\n", len);

    GList *ls = g_list_first(plist);
    while (ls != NULL)
    {
        CPDS_ZLOG_DEBUG("%s %d\n", ((NetData *)ls->data)->name, ((NetData *)ls->data)->stat);
        ls = g_list_next(ls);
    }
    
}

//TODO:该版本这里为获取多张网卡名功能测试代码，后须会完善获取状态并存入链表中
int get_multiple_netlink()
{
    GList *list = NULL;
    NetData *net;
    const char *stat;
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in6 *sa;
    struct sockaddr_in *sa4;
    char addr[INET6_ADDRSTRLEN];
    char addr4[INET_ADDRSTRLEN];
    int ret, val=RESULT_FAILED;

    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            sa = (struct sockaddr_in6 *)ifa->ifa_addr;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr,
                        sizeof(addr), NULL, 0, NI_NUMERICHOST);

            ret = get_netlink_status(ifa->ifa_name);
            if (ret != RESULT_FAILED)
            {
                net = (NetData *)malloc(sizeof(NetData));
                if (net == NULL)
                {
                    CPDS_ZLOG_ERROR("malloc failed");
                    goto out;
                }
                net->name = ifa->ifa_name;
                net->stat = ret;
                list = g_list_append(list, net);
                
                CPDS_ZLOG_DEBUG("NIC NAME: %s ", ifa->ifa_name);
                stat = ret == 1 ? "up" : "down";
                CPDS_ZLOG_DEBUG("Net link status: %s", stat);
            }
        }
        else
        {
            sa4 = (struct sockaddr_in *)ifa->ifa_addr;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), addr4,
                        sizeof(addr4), NULL, 0, NI_NUMERICHOST);

            ret = get_netlink_status(ifa->ifa_name);
            if (ret != RESULT_FAILED)
            {
                net = (NetData *)malloc(sizeof(NetData));
                if (net == NULL)
                {
                    CPDS_ZLOG_ERROR("malloc failed");
                    goto out;
                }
                net->name = ifa->ifa_name;
                net->stat = ret;
                list = g_list_append(list, net);

                CPDS_ZLOG_DEBUG("NIC NAME: %s ", ifa->ifa_name);
                stat = ret == 1 ? "up" : "down";
                CPDS_ZLOG_DEBUG("Net link status: %s", stat);
            }
        }
    }

    output_list(list);
    free_netlist(list);

    val = RESULT_SUCCESS;

out:
    freeifaddrs(ifap);
    return val;

}
