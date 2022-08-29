#ifndef AVG_CALCULATOR_H
#define AVG_CALCULATOR_H
#include "packets.h"
#include "collector.h"

enum e_MemAvgLabel
{
    RECV_BYTES = 0,
    RECV_PACKETS,
    SEND_BYTES,
    SEND_PACKETS
};

SCData* CalcCpuUtilizationAvg(uchar* collectedData, int cpuCnt, int maxCount, float toMs, int collectPeriod);
SCData* CalcMemAvg(uchar* collectedData, int maxCount);
SCData* CalcNetThroughputAvg(uchar* collectedData, int nicCount, int maxCount, int collectPeriod);

#endif