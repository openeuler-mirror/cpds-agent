#ifndef DATABASE_H
#define DATABASE_H
#include "../libs/sqlite3/sqlite3.h"


int create_sysinfotable(sqlite3 *db);
int add_record(sqlite3 *db, char *name, float data);

extern sqlite3 *db;
extern pthread_mutex_t mut;
#endif

