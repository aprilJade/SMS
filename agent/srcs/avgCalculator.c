#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "avgCalculator.h"

float RoundingOff(float x);

SCData* CalcCpuUtilizationAvg(uchar* collectedData, int cpuCnt, int maxCount, float toMs, int collectPeriod)
{
    static ulong* curIdle;
    static ulong* prevIdle;
    static float* deltaIdle;
    static float** cpuUtilizations;
    static int curCount;
    static int idx;
    static bool initialized;

    if (initialized == false)
    {
        curIdle = (ulong*)malloc(sizeof(ulong) * cpuCnt);
        prevIdle = (ulong*)malloc(sizeof(ulong) * cpuCnt);
        deltaIdle = (float*)malloc(sizeof(float) * cpuCnt);
        cpuUtilizations = (float**)malloc(sizeof(float*) * maxCount);
        for (int i = 0; i < maxCount; i++)
        {
            cpuUtilizations[i] = (float*)malloc(sizeof(float) * cpuCnt);
            memset(cpuUtilizations[i], 0, sizeof(float) * cpuCnt);
        }
        initialized = true;
    }
    
    SCData* result = (SCData*)malloc(sizeof(SCData));
    result->dataSize = sizeof(SHeader) + sizeof(SBodyAvgC) * cpuCnt;
    result->data = (uchar*)malloc(result->dataSize);
    memcpy(result->data, collectedData, sizeof(SHeader));

    SBodyc* hBody = (SBodyc*)(collectedData + sizeof(SHeader));
    SBodyAvgC* hAvgBody = (SBodyAvgC*)(result->data + sizeof(SHeader));
    float avg;
    for (int i = 0; i < cpuCnt; i++)
    {
        curIdle[i] = hBody[i].idleTime;
        deltaIdle[i] = (float)(curIdle[i] - prevIdle[i]) * toMs;
        prevIdle[i] = curIdle[i];
        cpuUtilizations[idx][i] = RoundingOff(100.0 - ((deltaIdle[i]) / (float)collectPeriod * 100.0));
        if (cpuUtilizations[idx][i] < 0)
            cpuUtilizations[idx][i] = 0;
        avg = 0.0;
        for (int j = 0; j < curCount; j++)
            avg += cpuUtilizations[j][i];
        hAvgBody[i].cpuUtilizationAvg = RoundingOff(avg / (float)curCount);
        hAvgBody[i].cpuUtilization = cpuUtilizations[idx][i];
        printf("%d: Cpu utilization average: %.2f%%\n", i, hAvgBody[i].cpuUtilizationAvg);
        printf("%d: CPU Usage: %.2f%%\n", i, hAvgBody[i].cpuUtilization);
    }
    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }
    if (curCount < maxCount)
        curCount++;
    
    return result;
}
