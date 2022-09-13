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
} SWorkTools;

int InsertCpuInfo(void* data, SWorkTools* tools);
int InsertCpuAvgInfo(void* data, SWorkTools* tools);
int InsertMemInfo(void* data, SWorkTools* tools);
int InsertMemAvgInfo(void* data, SWorkTools* tools);
int InsertNetInfo(void* data, SWorkTools* tools);
int InsertNetAvgInfo(void* data, SWorkTools* tools);
int InsertProcInfo(void* data, SWorkTools* tools);
int InsertDiskInfo(void* data, SWorkTools* tools);

SThreshold GetThresholds(SHashTable* options);

#endif