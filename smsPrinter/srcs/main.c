#include <stdio.h>
#include <stdlib.h>

#include "libpq-fe.h"

char* sql = "SELECT * FROM %s WHERE collect_time > \'%s\';";

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: ./printer [table name] [start datetime(\'yyyy-mm-dd\')] [end datetime(\'yyyy-mm-dd\')]\n");
        return 1;
    }

    PGconn* pgConn = PQconnectdb(" dbname = postgres ");
    if (PQstatus(pgConn) != CONNECTION_OK)
    {
        printf("fail to connection\nCheck the postgreSQL status\n");
        return 1;
    }

    printf("connected to postgreSQL server\n");

    char buf[512];
    sprintf(buf, "SELECT * FROM %s WHERE collect_time > \'%s\' AND collect_time < \'%s\';", argv[1], argv[2], argv[3]);

    printf("Querying below SQL.\n%s\n", buf);
    PGresult* result = PQexec(pgConn, buf);
    
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "Query failed\n");
        PQfinish(pgConn);
        PQclear(result);
        return 1;
    }

    printf("Query success!\nList up result.\n");
    
    printf("<%s>\n", argv[1]);
    int fieldsCnt = PQnfields(result);
    for (int i = 0; i < fieldsCnt; i++)
        printf("%-20s", PQfname(result, i));
    putchar('\n');

    for (int i = 0; i < PQntuples(result); i++)
    {
        for (int j = 0; j < fieldsCnt; j++)
            printf("%-20s", PQgetvalue(result, i, j));
        putchar('\n');
    }
    
    PQclear(result);
    return 0;
}