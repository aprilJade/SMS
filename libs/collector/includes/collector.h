#ifndef COLLECTOR_H
#define COLLECTOR_H

#define BUFFER_SIZE 2048

#include <stddef.h>
#include "packets.h"

typedef struct SCData
{
    uchar* data;
    int dataSize;
} SCData;

SCData* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf, int collectPeriod, char* agent_id);
SCData* CollectMemInfo(char* buf, int collectPeriod, char* agent_id);
SCData* CollectNetInfo(char* buf, int nicCount, int collectPeriod, char* agent_id);
SCData* CollectProcInfo(char *buf, uchar* dataBuf, int collectPeriod, char* agent_id);
SCData* CollectDiskInfo(char *buf, int diskDevCnt, int collectPeriod, char* agent_id);
void DestorySCData(SCData** target);

#endif