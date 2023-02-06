#ifndef MONITOR_H
#define MONITOR_H

#define  RESULT_FAILED -1
#define  RESULT_SUCCESS 0

#define  SYS_DISK_BUFF_LEN   256
#define  SYS_DISK_NAME_LEN   80
#define  SYS_100_PERSENT     100
#define  MAXBUFSIZE          1024
#define  WAIT_SECOND         3 

#define  FIELD_CPU_USAGE      "cpuusage"
#define  FIELD_DISK_USAGE     "DiskUsage"
#define FIELD_IO_WRITESIZE   "iowritesize"

#define CONFIG_PATH          "/etc/cpds/cpds-agent/cpds_log.conf"
#define CPDSCLASS_NAME       "cpds-agent"
typedef struct systeminfo {
    double cpuusage;
    float DiskUsage;
    float iowritesize;
}sys_t;

typedef int  data_t;
typedef char str_t;
typedef struct LNode {
    str_t   *name;
    data_t   stat;
}NetData;

typedef struct cpu_info {
    char name[20];                  
    unsigned int user;              
    unsigned int nice;              
    unsigned int system;           
    unsigned int idle;             
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
}_cpu_info;

void *get_sysinfo(void *arg);
void *push_sysinfo(void *arg);
float get_syscpu_usage();
double get_sysdisk_usage();
float get_sysio_wbs();

#endif