#ifndef DATABASE_H
#define DATABASE_H
#include "../util/sqlite3/sqlite3.h"

#define CREATE_SYSINFOTABLE "create table if not exists sysinfotable (id integer primary key, name text, value text);"
#define INSERT_INTO_SYSINFOTABLE "insert into sysinfotable (id, name, value) values (NULL, '%s', '%4.2f');"
#define DELETE_IN_TABLE_SYSINFO "delete from sysinfotable where id = %d;"
#define SELECT_SYSINFOTABLE "select * from sysinfotable;"

int create_sysinfotable(sqlite3 *db);
int add_record(sqlite3 *db, char *name, float data);
int display(void *para,int ncol,char *col_val[],char ** col_name);

extern sqlite3 *db;
#endif

