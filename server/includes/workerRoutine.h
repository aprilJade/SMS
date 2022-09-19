#ifndef WORKER_ROUTINE_H
#define WORKER_ROUTINE_H
#include "workerUtils.h"

#define QUERY_COUNT_THRESHOLD 256

typedef struct SWorkerParam
{
    SThreshold threshold;
    int workerId;
} SWorkerParam;

void* WorkerRoutine(void* param);

#endif