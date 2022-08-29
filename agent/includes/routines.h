#ifndef ROUTINES_H
#define ROUTINES_H
#define MIN_SLEEP_MS 500
#define ROUTINE_COUNT 5
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 4242

#include "packets.h"
#include "logger.h"
#include "Queue.h"
#include "tcpCtrl.h"

typedef struct SRoutineParam
{
    char agentId[16];
    uint collectPeriod;
} SRoutineParam;

int GetNicCount();

void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);
void* DiskInfoRoutine(void* param);
void* SendRoutine(void* param);

#endif