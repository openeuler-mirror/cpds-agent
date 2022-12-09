#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "monitor.h"
#include "database.h"
#include "cpdslog.h"
#include "../libs/sqlite3/sqlite3.h"

extern pthread_mutex_t mut;
int main()
{
    sys_t sys;
    int res;
    pthread_t getinfo_tid, pushinfo_tid;
    pthread_mutex_init(&mut, NULL);

    res = CPDS_ZLOG_INIT(CONFIG_PATH,CPDSCLASS_NAME);
    if(res)
    {
       printf("cpds zlog init fail!");
       CPDS_ZLOG_FINI();
       return RESULT_FAILED;
    }

    if(SQLITE_OK !=init_database())
    {
       printf("数据库初始化失败");
       return RESULT_FAILED;
    }

    res = pthread_create(&getinfo_tid, NULL, getSysinfo, (void *)&sys);
    if (res != 0)
    {
       printf("线程创建失败");
       return RESULT_FAILED; 
    }
    res = pthread_create(&pushinfo_tid, NULL, pushSysinfo, (void *)&sys);
    if( res != 0)
    {
       printf("线程创建失败");
       return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}