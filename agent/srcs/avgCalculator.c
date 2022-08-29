#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "avgCalculator.h"

static float RoundingOff(float x)
{
    x += 0.005;
    int tmp = (int)(x * 100);
    x = tmp / 100.0;
    return x;
}

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
    SHeader* hHeader = (SHeader*)result->data;
    hHeader->signature = SIGNATURE_AVG_CPU;

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
        // printf("%d: Cpu utilization average: %.2f%%\n", i, hAvgBody[i].cpuUtilizationAvg);
        // printf("%d: CPU Usage: %.2f%%\n", i, hAvgBody[i].cpuUtilization);
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

SCData* CalcMemAvg(uchar* collectedData, int maxCount)
{
    static int curCount;
    static int idx;
    static bool initialized;
    static float* memUsage;
    static float* swapUsage;

    if (initialized == false)
    {
        memUsage = (float*)malloc(sizeof(float) * maxCount);
        swapUsage = (float*)malloc(sizeof(float) * maxCount);
        initialized = true;
    }

    SCData* avgData = (SCData*)malloc(sizeof(SCData));
    avgData->dataSize = sizeof(SHeader) + sizeof(SBodyAvgM);
    avgData->data = (uchar*)malloc(avgData->dataSize);
    
    memcpy(avgData->data, collectedData, sizeof(SHeader));
    SHeader* hHeader = (SHeader*)avgData->data;
    hHeader->signature = SIGNATURE_AVG_MEM;

    SBodym* hBody = (SBodym*)(collectedData + sizeof(SHeader));
    SBodyAvgM* hAvgBody = (SBodyAvgM*)(avgData->data + sizeof(SHeader));
    
    memUsage[idx] = RoundingOff((float)(hBody->memTotal - hBody->memAvail) / (float)hBody->memTotal * 100);
    swapUsage[idx] = RoundingOff((float)(hBody->swapTotal - hBody->swapFree) / (float)hBody->swapTotal * 100);
    hAvgBody->memUsage = memUsage[idx];
    hAvgBody->swapUsage = swapUsage[idx];

    float memAvg = 0.0;
    float swapAvg = 0.0;
    for (int i = 0; i < curCount; i++)
    {
        memAvg += memUsage[i];
        swapAvg += swapUsage[i];
    }
    hAvgBody->memUsageAvg = RoundingOff(memAvg / (float)curCount);
    hAvgBody->swapUsageAvg = RoundingOff(swapAvg / (float)curCount);

    // printf("Memory usage: %.2f%%\n", hAvgBody->memUsage);
    // printf("Memory avg: %.2f%%\n", hAvgBody->memUsageAvg);
    // printf("Swap usage: %.2f%%\n", hAvgBody->swapUsage);
    // printf("Swap avg: %.2f%%\n", hAvgBody->swapUsageAvg);

    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }
    if (curCount < maxCount)
        curCount++;
    
    return avgData;
}