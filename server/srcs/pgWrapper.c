#include "pgWrapper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

SPgWrapper* NewPgWrapper(const char* conninfo)
{
    SPgWrapper* newWrapper = (SPgWrapper*)malloc(sizeof(SPgWrapper));
    
    newWrapper->connInfo = strdup(conninfo);
    newWrapper->connected = ConnectPg(newWrapper);
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

    res = PQexec(handle->conn, sql);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 1;
}