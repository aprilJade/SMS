#ifndef ROUTINES_H
#define ROUTINES_H
#define MIN_SLEEP_MS 500
#define ROUTINE_COUNT 4
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 4242

#include "packets.h"
#include "logger.h"
#include "tcpCtrl.h"

typedef struct SRoutineParam
{
    ulong collectorCreateTime;
    uint collectorID;
    uint collectPeriod;
    Queue* queue;
    Logger* logger;
} SRoutineParam;

typedef struct SSenderParam
{
    uint cpuPeriod;
    uint netPeriod;
    uint memPeriod;
    uint procPeriod;
    Queue* queue;
    Logger* logger;
    char host[16];
    short port;
} SSenderParam;

int GetNicCount();
SRoutineParam* GenRoutineParam(int collectPeriod, int collectorID, Queue* queue);

void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);
void* SendRoutine(void* param);

#endif