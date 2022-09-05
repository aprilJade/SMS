#ifndef PG_WRAPPER_H
#define PG_WRAPPER_H

#include <pthread.h>
#include <stdbool.h>
#include "libpq-fe.h"

typedef struct SPgWrapper
{
    PGconn* conn;
    bool connected;
    char* connInfo;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} SPgWrapper;

SPgWrapper* NewPgWrapper(const char* conninfo);
bool CheckPgStatus(SPgWrapper* db);
bool ConnectPg(SPgWrapper* wrapper);
int Query(SPgWrapper* handle, const char* sql);

#endif