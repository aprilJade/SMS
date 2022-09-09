#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "libpq-fe.h"

enum e_queryType
{
    SELECT_ALL,
    SELECT_AFTER_DATETIME,
    SELECT_DATETIME_RANGE
};

const char* const querys[] = {
    [0] = "SELECT * FROM %s;",
    [1] = "SELECT * FROM %s WHERE collect_time > \'%s\';",
    [2] = "SELECT * FROM %s WHERE collect_time > \'%s\' AND collect_time < \'%s\';"
};
const char* const tableNames[8] = {
    "cpu_informations",
    "cpu_avg_informations",
    "memory_informations",
    "memory_avg_informations",
    "network_informations",
    "network_avg_informations",
    "process_informations",
    "disk_informations"
};

void PrintTable(char* tableName, unsigned long elapseTime, PGresult* result)
{
    printf("<%s>\n", tableName);
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

    printf("Total %d row in %.2f ms\n", tuplesCnt, elapseTime / 1000.0);
}

int main(void)
{
    PGconn* pgConn = PQconnectdb(" dbname = postgres ");
    if (PQstatus(pgConn) != CONNECTION_OK)
    {
        printf("fail to connection\nCheck the postgreSQL status\n");
        return 1;
    }

    printf("connected to postgreSQL server\n");

    char ch;

    while (1)
    {
        puts("Select query type.");
        puts("1) SELECT * FROM [table name];");
        puts("2) SELECT * FROM [table name] WHERE collect_time > [datetime];");
        puts("3) SELECT * FROM [table name] WHERE collect_time > [datetime] AND collect_time [datetime];");
        puts("q) Quit");
        ch = getchar();
        if (ch >= '1' && ch <= '3')
            break;
        if (ch == 'q')
        {
            puts("Terminate SMS printer");
            PQfinish(pgConn);
            exit(0);
        }
        puts("Please just input 1 ~ 3 or q");
    }
    int queryType = ch - '1';


    while (1)
    {
        for (int i = 0; i < 8; i++)
            printf("%d) %s\n", i + 1, tableNames[i]);
        puts("q) Quit");

        ch = getchar();
        if (ch >= '1' && ch <= '8')
            break;
        if (ch == 'q')
        {
            puts("Terminate SMS printer");
            PQfinish(pgConn);
            exit(0);
        }
        puts("Please just input 1 ~ 8 or q");
    }
    int tableNameIdx = ch - '1';

    char buf[512];
    char datetime[2][32];

    switch (queryType)
    {
    case SELECT_ALL:
        sprintf(buf, "SELECT * FROM %s;", tableNames[tableNameIdx]);
        break;
    case SELECT_AFTER_DATETIME:
        puts("Input datetime as YYYY-MM-DD or YYYY-MM-DD HH:MM:SS format.");
        printf("datetime: ");
        scanf("%s", datetime[0]);
        sprintf(buf, "SELECT * FROM %s WHERE collect_time > \'%s\';", tableNames[tableNameIdx], datetime[0]);
        break;
    case SELECT_DATETIME_RANGE:
        puts("Input start datetime as YYYY-MM-DD or YYYY-MM-DD HH:MM:SS format.");
        printf("start datetime: ");
        scanf("%s", datetime[0]);
        puts("Input end datetime as YYYY-MM-DD or YYYY-MM-DD HH:MM:SS format.");
        printf("end datetime: ");
        scanf("%s", datetime[1]);
        sprintf(buf, "SELECT * FROM %s WHERE collect_time > \'%s\' AND collect_time < \'%s\';",
            tableNames[tableNameIdx], datetime[0], datetime[1]);
        break;
    }

    printf("Querying below SQL. If you want to proceed query enter y and if not, enter n\n%s\n", buf);
    printf("y or n: ");
    getchar();
    ch = getchar();
    if (ch == 'n')
    {
        puts("Terminate sms printer.");
        PQfinish(pgConn);
        return 0;
    }

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
    gettimeofday(tv, NULL);
    unsigned long elapsedTime = (tv->tv_sec * 1000000 + tv->tv_usec) - prevTime;  

    printf("Query success!\nTime spent query: %.2f ms\nRow count: %d\n", elapsedTime / 1000.0, PQntuples(result));
    printf("Enter any key...");
    getchar();
    PrintTable((char*)tableNames[tableNameIdx], elapsedTime, result);

    PQclear(result);
    PQfinish(pgConn);
    return 0;
}