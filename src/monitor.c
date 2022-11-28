#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "monitor.h"
#include "../util/sqlite3/sqlite3.h"

int main()
{
    sys_t sys;
    int res;
    pthread_t getinfo_tid, pushinfo_tid;

    sqlite3 *db = NULL;
    if(sqlite3_open("test.db", &db) != SQLITE_OK)
    {
       printf("open db faild:%s", sqlite3_errmsg(db));
       return RESULT_FAILED; 
    }
    res = pthread_create(&getinfo_tid, NULL, getSysinfo,(void *)&sys);
    if (res != 0)
    {
       printf("线程创建失败");
       return RESULT_FAILED; 
    }
    res = pthread_create(&pushinfo_tid, NULL, pushSysinfo,(void *)&sys);
    if( res != 0)
    {
       printf("线程创建失败");
       return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}