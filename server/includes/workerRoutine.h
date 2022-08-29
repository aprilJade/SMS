#ifndef WORKER_ROUTINE_H
#define WORKER_ROUTINE_H
#include "logger.h"
#include "packets.h"
#include "Queue.h"
#include "pgWrapper.h"

typedef struct SWorkerParam
{
    SPgWrapper* db;
    int workerId;
} SWorkerParam;

typedef struct SWorkTools
{
    SPgWrapper* dbWrapper;
    int workerId;
} SWorkTools;

void* WorkerRoutine(void* param);

#endif