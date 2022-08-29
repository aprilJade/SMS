#ifndef AVG_CALCULATOR_H
#define AVG_CALCULATOR_H
#include "packets.h"
#include "collector.h"

SCData* CalcCpuUtilizationAvg(uchar* collectedData, int cpuCnt, int maxCount, float toMs, int collectPeriod);

#endif