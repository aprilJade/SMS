#ifndef PG_WRAPPER_H
#define PG_WRAPPER_H

#include <pthread.h>
#include "libpq-fe.h"

typedef struct SPgWrapper
{
    PGconn* conn;
    pthread_mutex_t lock;
} SPgWrapper;

PGconn* NewPsql();
int Query(SPgWrapper* conn, const char* sql);

#endif