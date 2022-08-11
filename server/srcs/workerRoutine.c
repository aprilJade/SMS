#include "workerRoutine.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


// Calc delta value, average value
// And save to DB
void WorkCpuInfo(void* data)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyc* hBody;
    return ;

    printf("signature: %x, body count: %d, body size: %d, collect period: %d collect time: %ld\n",
        hHeader->signature,
        hHeader->bodyCount,
        hHeader->bodySize,
        hHeader->collectPeriod,
        hHeader->collectTime);
    hBody = (SBodyc*)(data + sizeof(SHeader));
    for (int i = 0; i < hHeader->bodyCount; i++)
        printf("%d: %010ld %010ld %010ld %010ld\n",
            i,
            hBody[i].usrCpuRunTime,
            hBody[i].sysCpuRunTime,
            hBody[i].idleTime,
            hBody[i].waitTime);
}

void WorkMemInfo(void* data)
{
    SHeader* hHeader = (SHeader*)data;
    SBodym* hBody;
    return ;
    printf("signature: %x, body count: %d, body size: %d, collect period: %d\n",
        hHeader->signature,
        hHeader->bodyCount,
        hHeader->bodySize,
        hHeader->collectPeriod);
    hBody = (SBodym*)(data + sizeof(SHeader));
    for (int i = 0; i < hHeader->bodyCount; i++)
        printf("%d: %08d kB %08d kB %08d kB %08d kB %08d kB %08d kB\n",
            i,
            hBody[i].memTotal,
            hBody[i].memFree,
            hBody[i].memUsed,
            hBody[i].memAvail,
            hBody[i].swapTotal,
            hBody[i].swapFree);
}

void WorkNetInfo(void* data)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyn* hBody;
    char nicName[16];
    return ;

    printf("signature: %x, body count: %d, body size: %d, collect period: %d\n",
        hHeader->signature,
        hHeader->bodyCount,
        hHeader->bodySize,
        hHeader->collectPeriod);
    hBody = (SBodyn*)(data + sizeof(SHeader));
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        //strncpy(nicName, hBody[i].name, hBody[i].nameLength);
        printf("%d: %ld B %ld %ld B %ld %d\n",
            i,
            hBody[i].recvBytes,
            hBody[i].recvPackets,
            hBody[i].sendBytes,
            hBody[i].sendPackets,
            hBody[i].nameLength);
    }
}

void WorkProcInfo(void* data)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyp* hBody;
    return ;

    printf("signature: %x, body count: %d, body size: %d, collect period: %d\n",
        hHeader->signature,
        hHeader->bodyCount,
        hHeader->bodySize,
        hHeader->collectPeriod);
}

void* WorkerRoutine(void* param)
{
    SWorkerParam* pParam = (SWorkerParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    char logMsg[128];
    SHeader* hHeader;
    void (*work)(void*);
    void* data;

    while (1)
    {
        sprintf(logMsg, "%d worker-ready", pParam->workerId);
        Log(logger, logMsg);
        pthread_mutex_lock(&queue->lock);
        while (IsEmpty(queue))
        {
            pthread_mutex_unlock(&queue->lock);
            usleep(500);
            pthread_mutex_lock(&queue->lock);
        }
        data = Pop(queue);
        pthread_mutex_unlock(&queue->lock);

        sprintf(logMsg, "%d work-start", pParam->workerId);
        Log(logger, logMsg);

        hHeader = (SHeader*)data;
        char pktId = hHeader->signature & EXTRACT_SIGNATURE;
        switch(pktId)
        {
        case 'c':
            WorkCpuInfo(data);
            break;
        case 'm':
            WorkMemInfo(data);
            break;
        case 'n':
            WorkNetInfo(data);
            break;
        case 'p':
            WorkProcInfo(data);
            break;
        }
        free(data);
        sprintf(logMsg, "%d work-done %c", pParam->workerId, pktId);
        Log(logger, logMsg);
    }
}