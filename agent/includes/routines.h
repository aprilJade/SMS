#ifndef ROUTINES_H
#define ROUTINES_H
#define MIN_SLEEP_MS 500

#include "packets.h"
#include "logger.h"
#include "Queue.h"
#include "tcpCtrl.h"

enum e_collectorIdx
{
    CPU_COLLECTOR_ID = 0,
    MEM_COLLECTOR_ID,
    NET_COLLECTOR_ID,
    PROC_COLLECTOR_ID,
    DISK_COLLECTOR_ID
};

void* CpuInfoRoutine(void* param);
void* MemInfoRoutine(void* param);
void* NetInfoRoutine(void* param);
void* ProcInfoRoutine(void* param);
void* DiskInfoRoutine(void* param);
void* SendRoutine(void* param);

#endif