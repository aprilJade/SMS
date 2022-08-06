#ifndef COLLECT_ROUTINE_H
#define COLLECT_ROUTINE_H
#define MIN_SLEEP_MS 500
#define ROUTINE_COUNT 1

#include "packets.h"

typedef struct SRoutineParam
{
    //ulonglong agentStartTime;
    uint collectPeriod;
} SRoutineParam;

SRoutineParam* GenRoutineParam(int collectPeriod);
void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);

void* CpuInfoSendRoutine(void* param);
void* MemInfoSendRoutine(void* param);
void* NetInfoSendRoutine(void* param);
void* ProcInfoSendRoutine(void* param);


#endif