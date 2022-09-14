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
} SPgWrapper;

SPgWrapper* NewPgWrapper(const char* conninfo);
bool CheckPgStatus(SPgWrapper* db);
bool ConnectPg(SPgWrapper* wrapper);
bool TryConectPg(SPgWrapper* wrapper, int tryCnt, int tryPeriodSec);
int Query(SPgWrapper* handle, const char* sql);

#endif