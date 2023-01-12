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
    int val = RESULT_FAILED;
    pthread_t getinfo_tid, pushinfo_tid;
    pthread_mutex_init(&mut, NULL);

   
    res = CPDS_ZLOG_INIT(CONFIG_PATH,CPDSCLASS_NAME);
    if (res) {
       printf("cpds-zlog failed to be initialized");
       goto out;
    }

    CPDS_ZLOG_INFO("Welcome in cpds-agent");

    if (SQLITE_OK != init_database()) {
       CPDS_ZLOG_ERROR("failed to initialize the database");
       goto clean_log;
    }

    res = pthread_create(&getinfo_tid, NULL, get_sysinfo, (void *)&sys);
    if (res != 0) {
       CPDS_ZLOG_ERROR("pthread creation failure");
       goto clean_log; 
    }
    res = pthread_create(&pushinfo_tid, NULL, push_sysinfo, (void *)&sys);
    if (res != 0) {
       CPDS_ZLOG_ERROR("pthread creation failure");
       goto clean_log;
    }
   
    val = RESULT_SUCCESS;
clean_log:
    CPDS_ZLOG_FINI();
out:
    return val;
}