#ifndef WORKER_UTILS_H
#define WORKER_UTILS_H
#include "pgWrapper.h"
#include "packets.h"
#include "confParser.h"

typedef struct SThreshold
{
    float cpuUtilization;
    float memUsage;
    float swapUsage;
    ulong sendBytes;
    ulong recvBytes;
} SThreshold;

typedef struct SWorkTools
{
    SPgWrapper* dbWrapper;
    SThreshold threshold;
    int workerId;
    int queriedSqlCnt;
} SWorkTools;

typedef struct SSqlVec
{
    char** data;
    int dataCnt;
    int maxCnt;
} SSqlVec;

void NewSqlQueue(SSqlVec* vec, int maxCnt);
int AddTailSql(char* sql, SSqlVec* vec);
char* GetHeadSql(SSqlVec* vec);

int InsertCpuInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertCpuAvgInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertMemInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertMemAvgInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertNetInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertNetAvgInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertProcInfo(char* sqlBuffer, void* data, SWorkTools* tools);
int InsertDiskInfo(char* sqlBuffer, void* data, SWorkTools* tools);

SThreshold GetThresholds(SHashTable* options);

#endif