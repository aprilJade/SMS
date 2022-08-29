#ifndef ROUTINES_H
#define ROUTINES_H
#define MIN_SLEEP_MS 500

#include "packets.h"
#include "logger.h"
#include "Queue.h"
#include "tcpCtrl.h"

typedef struct SRoutineParam
{
    char agentId[16];
    uint collectPeriod;
} SRoutineParam;

int GetNicCount();  // ?? Move to appropriate file

void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);
void* DiskInfoRoutine(void* param);
void* SendRoutine(void* param);

#endif