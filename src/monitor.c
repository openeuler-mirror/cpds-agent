#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "monitor.h"

int main()
{
    sys_t sys;
    int res;
    pthread_t getinfo_tid, pushinfo_tid;

    res = pthread_create(&getinfo_tid, NULL, getSysinfo,(void *)&sys);
    if (res != 0)
    {
       printf("线程创建失败");
       return 0; 
    }
    res = pthread_create(&pushinfo_tid, NULL, pushSysinfo,(void *)&sys);
    if( res != 0)
    {
       printf("线程创建失败");
       return 0;
    }

    return 0;
}