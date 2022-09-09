#ifndef WORKER_ROUTINE_H
#define WORKER_ROUTINE_H
#include "logger.h"
#include "packets.h"
#include "Queue.h"
#include "pgWrapper.h"
#include "confParser.h"

typedef struct SThreshold
{
    float cpuUtilization;
    float memUsage;
    float swapUsage;
    uint netThroughput;
} SThreshold;

typedef struct SWorkerParam
{
    SPgWrapper* db;
    SThreshold threshold;
    int workerId;
} SWorkerParam;

typedef struct SWorkTools
{
    SPgWrapper* dbWrapper;
    SThreshold threshold;
    int workerId;
} SWorkTools;

void* WorkerRoutine(void* param);

#endif