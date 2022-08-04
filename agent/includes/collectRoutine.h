#ifndef COLLECT_ROUTINE_H
#define COLLECT_ROUTINE_H
#include "packets.h"

typedef struct SRoutineParam
{
    ulonglong agentStartTime;
    uint collectPeriod;
} SRoutineParam;

void initRoutineParam(SRoutineParam* target);
void* cpuInfoRoutine(void* param);
void* memInfoRoutine(void* param);

#endif