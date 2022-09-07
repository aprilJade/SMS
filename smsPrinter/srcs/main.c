#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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
    
    unsigned long prevTime;
    struct timeval* tv;
    gettimeofday(tv, NULL);
    prevTime = tv->tv_sec * 1000000 + tv->tv_usec;
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
    for (int i = 0; i < fieldsCnt * 20; i++)
        putchar('-');
    putchar('\n');
    

    int tuplesCnt = PQntuples(result);
    for (int i = 0; i < tuplesCnt; i++)
    {
        for (int j = 0; j < fieldsCnt; j++)
            printf("%-20s", PQgetvalue(result, i, j));
        putchar('\n');
    }

    gettimeofday(tv, NULL);
    unsigned long elapsedTime = (tv->tv_sec * 1000000 + tv->tv_usec) - prevTime;   
    printf("Total %d row in %.2f ms\n", tuplesCnt, elapsedTime / 1000.0);
    PQclear(result);
    return 0;
}