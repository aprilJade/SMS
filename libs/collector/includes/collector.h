#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stddef.h>
#include "packets.h"

#define BUFFER_SIZE 2048

typedef struct SCData
{
    uchar* data;
    int dataSize;
} SCData;

enum e_MemAvgLabel
{
    RECV_BYTES = 0,
    RECV_PACKETS,
    SEND_BYTES,
    SEND_PACKETS
};

SCData* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf, int collectPeriod, char* agent_id);
SCData* CollectMemInfo(char* buf, int collectPeriod, char* agent_id);
SCData* CollectNetInfo(char* buf, int nicCount, int collectPeriod, char* agent_id);
SCData* CollectProcInfo(char *buf, uchar* dataBuf, int collectPeriod, char* agent_id);
SCData* CollectDiskInfo(char *buf, int diskDevCnt, int collectPeriod, char* agent_id);
SCData* CalcCpuUtilizationAvg(uchar* collectedData, int cpuCnt, int maxCount, float toMs, int collectPeriod);
SCData* CalcMemAvg(uchar* collectedData, int maxCount);
SCData* CalcNetThroughputAvg(uchar* collectedData, int nicCount, int maxCount, int collectPeriod);

void DestorySCData(SCData** target);

#endif