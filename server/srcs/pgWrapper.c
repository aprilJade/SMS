#include "pgWrapper.h"
#include <stdlib.h>
// #include "libpq-fe.h"

SPgWrapper* NewPgWrapper(const char* conninfo)
{
    SPgWrapper* newWrapper = (SPgWrapper*)malloc(sizeof(SPgWrapper));

    pthread_mutex_init(&newWrapper->lock, NULL);
    newWrapper->conn = PQconnectdb(conninfo);
    if (PQstatus(newWrapper->conn) != CONNECTION_OK)
    {
        // TODO: handle connection error
        fprintf(stderr, "DB: connection fail\n");
        free(newWrapper);
        return NULL;
    }
    return newWrapper;
}

int Query(SPgWrapper* handle, const char* sql)
{
    PGresult* res;
    pthread_mutex_lock(&handle->lock);
    res = PQexec(handle->conn, sql);
    pthread_mutex_unlock(&handle->lock);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 1;
}