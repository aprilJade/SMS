#ifndef PG_WRAPPER_H
#define PG_WRAPPER_H

#include <pthread.h>
#include "libpq-fe.h"

typedef struct SPgWrapper
{
    PGconn* conn;
    pthread_mutex_t lock;
} SPgWrapper;

SPgWrapper* NewPgWrapper(const char* conninfo);
int Query(SPgWrapper* handle, const char* sql);

#endif