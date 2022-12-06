#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "monitor.h"
#include "database.h"
#include "../libs/sqlite3/sqlite3.h"

#define CREATE_SYSINFOTABLE "create table if not exists sysinfotable (id integer primary key, name text, value text);"
#define INSERT_INTO_SYSINFOTABLE "insert into sysinfotable (id, name, value) values (NULL, '%s', '%4.2f');"
#define DELETE_IN_TABLE_SYSINFO "delete from sysinfotable where id = %d;"
#define SELECT_SYSINFOTABLE "select * from sysinfotable;"

static sqlite3 *pdb;
int create_sysinfotable()
{
    char *errmsg = NULL;

    if (SQLITE_OK != sqlite3_exec(pdb, CREATE_SYSINFOTABLE, NULL, NULL, &errmsg))
    {
        printf("create table error!%s\n", errmsg);
        sqlite3_free(errmsg);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

int init_database()
{
    if(sqlite3_open("test.db", &pdb) != SQLITE_OK)
    {
       printf("open pdb faild:%s", sqlite3_errmsg(pdb));
       return RESULT_FAILED; 
    }

    if (SQLITE_OK != create_sysinfotable(pdb))
    {
       printf("create table failed!\n");
       sqlite3_close(pdb);
       return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

int add_record(char *name, float data)
{
    char sql[128];
    char *errmsg = NULL;

    sprintf(sql, INSERT_INTO_SYSINFOTABLE, name, data);
    if (SQLITE_OK != sqlite3_exec(pdb, sql, NULL, NULL, &errmsg))
    {
        printf("add record fail!%s\n", errmsg);
        sqlite3_free(errmsg);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}
//TODO:该版本这里为功能测试代码输入删除信息，后续会做成接口获取删除信息
int delete_record(int id)
{
    char sql[128];
    char *errmsg = NULL;

    sprintf(sql, DELETE_IN_TABLE_SYSINFO, id);
    if (SQLITE_OK != sqlite3_exec(pdb, sql, NULL, NULL, &errmsg))
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
//TODO:该版本这里为功能测试代码，查询后，用回调函数显示,后续测试完删除
int inquire_uscb()
{
    char *errmsg = NULL;
    int flag = 0;

    if (SQLITE_OK != sqlite3_exec(pdb, SELECT_SYSINFOTABLE, display, (void *)&flag, &errmsg))
    {
        printf("select fail!%s\n", errmsg);
        sqlite3_free(errmsg);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}