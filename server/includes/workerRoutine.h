#ifndef WORKER_ROUTINE_H
#define WORKER_ROUTINE_H
#include "workerUtils.h"

typedef struct SWorkerParam
{
    SPgWrapper* db;
    SThreshold threshold;
    int workerId;
} SWorkerParam;


void* WorkerRoutine(void* param);

#endif