#include "pgWrapper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

int pgsignal(int signo)
{
    puts("pipe broken");
}

static void* PgManageRoutine(void* param)
{
    SPgWrapper* wrapper = (SPgWrapper*)param;
    signal(SIGPIPE, pgsignal);

    while (1)
    {
        if (CheckPgStatus(wrapper) == true)
        {
            puts("connect ok");
            sleep(1);
            continue;
        }

        if (CheckPgStatus(wrapper) == false)
        {
            while (ConnectPg(wrapper) == false)
            {
                puts("try conn");
                sleep(1);
            }
            puts("wake up!!!");
            pthread_cond_signal(&wrapper->cond);
            continue;
        }
    }
}

SPgWrapper* NewPgWrapper(const char* conninfo)
{
    SPgWrapper* newWrapper = (SPgWrapper*)malloc(sizeof(SPgWrapper));

    pthread_mutex_init(&newWrapper->lock, NULL);
    pthread_cond_init(&newWrapper->cond, NULL);
    newWrapper->connInfo = strdup(conninfo);
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