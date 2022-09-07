#include "pgWrapper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

extern bool g_turnOff;

static void* PgManageRoutine(void* param)
{
    SPgWrapper* wrapper = (SPgWrapper*)param;
    // printf("%d\n", wrapper->connected);

    while (1)
    {
        if (g_turnOff == true)
            break;
        if (wrapper->connected == true)
        {
            sleep(1);
            continue;
        }

        while (wrapper->connected == false)
        {
            if (g_turnOff)
                break;
            PQreset(wrapper->conn);
            wrapper->connected = CheckPgStatus(wrapper);
            sleep(1);
        }
        wrapper->connected = true;
        pthread_mutex_lock(&wrapper->lock);
        pthread_cond_broadcast(&wrapper->cond);
        pthread_mutex_unlock(&wrapper->lock);
    }

    // when g_turnOff is true.
    pthread_mutex_lock(&wrapper->lock);
    pthread_cond_broadcast(&wrapper->cond);
    pthread_mutex_unlock(&wrapper->lock);
}

SPgWrapper* NewPgWrapper(const char* conninfo)
{
    SPgWrapper* newWrapper = (SPgWrapper*)malloc(sizeof(SPgWrapper));
    
    pthread_mutex_init(&newWrapper->lock, NULL);
    pthread_cond_init(&newWrapper->cond, NULL);
    newWrapper->connInfo = strdup(conninfo);
    newWrapper->connected = ConnectPg(newWrapper);
    pthread_t tid;
    pthread_create(&tid, NULL, PgManageRoutine, newWrapper);
    return newWrapper;
}

bool ConnectPg(SPgWrapper* wrapper)
{
    wrapper->conn = PQconnectdb(wrapper->connInfo);
    return CheckPgStatus(wrapper);
}

bool CheckPgStatus(SPgWrapper* db)
{
    return PQstatus(db->conn) == CONNECTION_OK;
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