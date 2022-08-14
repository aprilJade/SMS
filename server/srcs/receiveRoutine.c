#include "receiveRoutine.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define DETAIL_PRINT_CPU 0
#define DETAIL_PRINT_MEM 0
#define DETAIL_PRINT_NET 0
#define DETAIL_PRINT_PROC 0

int IsValidSignature(int signature)
{
    if (signature == SIGNATURE_CPU)
        return 1;
    if (signature == SIGNATURE_MEM)
        return 1;
    if (signature == SIGNATURE_NET)
        return 1;
    if (signature == SIGNATURE_PROC)
        return 1;
    return 0;
}

void* ReceiveRoutine(void* param)
{
    SReceiveParam* pParam = (SReceiveParam*)param;
    int readSize;
    char buf[1024 * 1024] = { 0, };
    Logger* logger = pParam->logger;
    Queue* queue = pParam->queue;
    char logMsg[128];
    char* pb;
    int packetSize;

    SHeader* hHeader;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 1024 * 1024)) == -1)
        {
            sprintf(logMsg, "fail to receive from %s:%d", pParam->host, pParam->port);
            Log(logger, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb = buf;

        if (readSize == 0)
        {
            sprintf(logMsg, "disconnected %s:%d", pParam->host, pParam->port);
            Log(logger, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb[readSize] = 0;


        while (readSize > 0)
        {
            printf("%d\n", readSize);
            hHeader = (SHeader*)pb;
            if (!IsValidSignature(hHeader->signature))
            {
                sprintf(logMsg, "invalid packet from %s:%d %d B %x",
                    pParam->host,
                    pParam->port,
                    readSize,
                    hHeader->signature);
                Log(logger, logMsg);
                close(pParam->clientSock);
                break;
            }

            sprintf(logMsg, "received from %s:%d %d B %x",
                pParam->host,
                pParam->port,
                readSize,
                hHeader->signature);
            Log(logger, logMsg);

            packetSize = (hHeader->bodyCount * hHeader->bodySize + sizeof(SHeader));
            
            uchar* receivedData = (uchar*)malloc(sizeof(uchar) * packetSize);
            memcpy(receivedData, pb, packetSize);
            readSize -= packetSize;
            pb += packetSize;
            pthread_mutex_lock(&queue->lock);
            Push(receivedData, queue);
            pthread_mutex_unlock(&queue->lock);
        }
    }
    sprintf(logMsg, "kill receiver for %s:%d", pParam->host, pParam->port);
    Log(logger, logMsg);
}