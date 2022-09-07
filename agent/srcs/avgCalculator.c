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
        cpuUtilizations = (float**)malloc(sizeof(float*) * cpuCnt);
        for (int i = 0; i < cpuCnt; i++)
        {
            cpuUtilizations[i] = (float*)malloc(sizeof(float) * maxCount);
            memset(cpuUtilizations[i], 0, sizeof(float) * maxCount);
        }
        initialized = true;
    }
    
    SCData* result = (SCData*)malloc(sizeof(SCData));
    result->dataSize = sizeof(SHeader) + sizeof(SBodyAvgC) * cpuCnt;
    result->data = (uchar*)malloc(result->dataSize);
    memcpy(result->data, collectedData, sizeof(SHeader));
    SHeader* hHeader = (SHeader*)result->data;
    hHeader->signature = SIGNATURE_AVG_CPU;
    hHeader->bodySize = sizeof(SBodyAvgC);

    SBodyc* hBody = (SBodyc*)(collectedData + sizeof(SHeader));
    SBodyAvgC* hAvgBody = (SBodyAvgC*)(result->data + sizeof(SHeader));
    float avg;
    for (int i = 0; i < cpuCnt; i++)
    {
        curIdle[i] = hBody[i].idleTime;
        deltaIdle[i] = (float)(curIdle[i] - prevIdle[i]) * 10;
        prevIdle[i] = curIdle[i];
        cpuUtilizations[i][idx] = RoundingOff(100.0 - ((deltaIdle[i]) / (float)collectPeriod * 100.0));
        avg = 0.0;
        for (int j = 0; j < curCount; j++)
            avg += cpuUtilizations[i][j];
        hAvgBody[i].cpuUtilizationAvg = RoundingOff(avg / (float)curCount);
        hAvgBody[i].cpuUtilization = cpuUtilizations[i][idx];
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
    hHeader->bodySize = sizeof(SBodyAvgM);

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

    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }
    if (curCount < maxCount)
        curCount++;
    
    return avgData;
}

SCData* CalcNetThroughputAvg(uchar* collectedData, int nicCount, int maxCount, int collectPeriod)
{
    static int idx;
    static int curCount;
    static bool initialized;
    static ulong* prevVal[4];
    static ulong* curVal[4];
    static float** deltaVal[4];

    if (initialized == false)
    {
        for (int i = 0; i < 4; i++)
        {
            prevVal[i] = (ulong*)malloc(sizeof(ulong) * nicCount);
            curVal[i] = (ulong*)malloc(sizeof(ulong) * nicCount);
            deltaVal[i] = (float**)malloc(sizeof(float*) * nicCount);
            for (int j = 0; j < nicCount; j++)
            {
                deltaVal[i][j] = (float*)malloc(sizeof(float) * maxCount);
                memset(deltaVal[i][j], 0, sizeof(float) * maxCount);
            }

        }
        initialized = true;
    }
    
    SBodyn* hBody = (SBodyn*)(collectedData + sizeof(SHeader));

    SCData* avgData = (SCData*)malloc(sizeof(SCData));
    avgData->dataSize = sizeof(SHeader) + sizeof(SBodyAvgN) * nicCount;
    avgData->data = (uchar*)malloc(avgData->dataSize);
    memcpy(avgData->data, collectedData, sizeof(SHeader));

    SHeader* hHeader = (SHeader*)avgData->data;
    hHeader->signature = SIGNATURE_AVG_NET;
    hHeader->bodySize = sizeof(SBodyAvgN);

    SBodyAvgN* hAvgBody = (SBodyAvgN*)(avgData->data + sizeof(SHeader));
    float sum = 0.0;
    for (int i = 0; i < nicCount; i++)
    {
        hAvgBody[i].nameLength = hBody[i].nameLength;
        memcpy(hAvgBody[i].name, hBody[i].name, hBody[i].nameLength);

        curVal[RECV_BYTES][i] = hBody[i].recvBytes;
        deltaVal[RECV_BYTES][i][idx] = RoundingOff((float)(curVal[RECV_BYTES][i] - prevVal[RECV_BYTES][i]) / (float)collectPeriod * 1000.0);
        prevVal[RECV_BYTES][i] = curVal[RECV_BYTES][i];
        hAvgBody[i].recvBytesPerSec = deltaVal[RECV_BYTES][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[RECV_BYTES][i][j];
        hAvgBody[i].recvBytesPerSecAvg = sum / curCount;

        curVal[RECV_PACKETS][i] = hBody[i].recvPackets;
        deltaVal[RECV_PACKETS][i][idx] = RoundingOff((float)(curVal[RECV_PACKETS][i] - prevVal[RECV_PACKETS][i]) / (float)collectPeriod * 1000.0);
        prevVal[RECV_PACKETS][i] = curVal[RECV_PACKETS][i];
        hAvgBody[i].recvPacketsPerSec = deltaVal[RECV_PACKETS][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[RECV_PACKETS][i][j];
        hAvgBody[i].recvPacketsPerSecAvg = sum / curCount;

        curVal[SEND_BYTES][i] = hBody[i].sendBytes;
        deltaVal[SEND_BYTES][i][idx] = RoundingOff((float)(curVal[SEND_BYTES][i] - prevVal[SEND_BYTES][i]) / (float)collectPeriod * 1000.0);
        prevVal[SEND_BYTES][i] = curVal[SEND_BYTES][i];
        hAvgBody[i].sendBytesPerSec = deltaVal[SEND_BYTES][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[RECV_BYTES][i][j];
        hAvgBody[i].sendBytesPerSecAvg = sum / curCount;

        curVal[SEND_PACKETS][i] = hBody[i].sendPackets;
        deltaVal[SEND_PACKETS][i][idx] = RoundingOff((float)(curVal[SEND_PACKETS][i] - prevVal[SEND_PACKETS][i]) / (float)collectPeriod * 1000.0);
        prevVal[SEND_PACKETS][i] = curVal[SEND_PACKETS][i];
        hAvgBody[i].sendPacketsPerSec = deltaVal[SEND_PACKETS][i][idx];

        sum = 0.0;
        for (int j = 0; j < curCount; j++)
            sum += deltaVal[SEND_PACKETS][i][j];
        hAvgBody[i].sendPacketsPerSecAvg = sum / curCount;
    }
    
    if (curCount)
    {
        idx++;
        idx %= maxCount;
    }

    if (curCount < maxCount)
        curCount++;
    
    return avgData;
}