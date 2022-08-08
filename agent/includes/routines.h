#ifndef ROUTINES_H
#define ROUTINES_H
#define MIN_SLEEP_MS 500
#define ROUTINE_COUNT 4
#define HOST "127.0.0.1"
#define PORT 4244

#include "packets.h"
#include "logger.h"

typedef struct SRoutineParam
{
    ulong collectorCreateTime;
    uint collectorID;
    uint collectPeriod;
    Queue* queue;
    Logger* logger;
} SRoutineParam;

int GetNicCount();
SRoutineParam* GenRoutineParam(int collectPeriod, int collectorID);
void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);

void* SendRoutine(void* param);

#endif