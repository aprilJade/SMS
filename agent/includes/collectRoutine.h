#ifndef COLLECT_ROUTINE_H
#define COLLECT_ROUTINE_H
#define MIN_SLEEP_MS 500
#define ROUTINE_COUNT 1

#include "packets.h"
#include <Queue.h>

typedef struct SRoutineParam
{
    ulong collectorCreateTime;
    uint collectorID;
    uint collectPeriod;
    ConcurrentQueue* queue;
} SRoutineParam;

SRoutineParam* GenRoutineParam(int collectPeriod, int collectorID);
void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);

void* SendRoutine(void* param);

#endif