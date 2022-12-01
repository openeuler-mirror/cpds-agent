#ifndef DATABASE_H
#define DATABASE_H
#include "../util/sqlite3/sqlite3.h"

#define INSERT_INTO_SYSINFOTABLE "insert into sysinfotable (id, name, value) values (NULL, '%s', '%4.2f');"

int create_sysinfotable(sqlite3 *db);
int add_record(sqlite3 *db, char *name, float data);

#endif

