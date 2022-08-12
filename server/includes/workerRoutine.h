#ifndef WORKER_ROUTINE_H
#define WORKER_ROUTINE_H
#include "logger.h"
#include "packets.h"
#include "Queue.h"
#include "pgWrapper.h"

typedef struct SWorkerParam
{
    Logger* logger;
    Queue* queue;
    SPgWrapper* db;
    int workerId;
} SWorkerParam;

void* WorkerRoutine(void* param);

#endif