#ifndef COLLECT_ROUTINE_H
#define COLLECT_ROUTINE_H
#include "packets.h"
#define SIGNATURE_CPU "SMSc"
#define SIGNATURE_MEM "SMSm"
#define SIGNATURE_NET "SMSn"
#define SIGNATURE_PROC "SMSp"

typedef struct SRoutineParam
{
    ulonglong agentStartTime;
    uint collectPeriod;
} SRoutineParam;

void initRoutineParam(SRoutineParam* target);
void* cpuInfoRoutine(void* param);
void* memInfoRoutine(void* param);
void* netInfoRoutine(void* param);
void* procInfoRoutine(void* param);

#endif