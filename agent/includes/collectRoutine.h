#ifndef COLLECT_ROUTINE_H
#define COLLECT_ROUTINE_H
#include "packets.h"

typedef struct SRoutineParam
{
    ulonglong agentStartTime;
    uint collectPeriod;
} SRoutineParam;

void InitRoutineParam(SRoutineParam* target);
void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);

#endif