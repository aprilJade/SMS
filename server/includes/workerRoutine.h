#ifndef WORKER_ROUTINE_H
#define WORKER_ROUTINE_H
#include "logger.h"
#include "packets.h"
#include "Queue.h"

typedef struct SWorkerParam
{
    Logger* logger;
    Queue* queue;
    int workerId;
} SWorkerParam;

void* WorkerRoutine(void* param);

#endif