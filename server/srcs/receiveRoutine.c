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
    char buf[1024 * 48] = { 0, };
    Logger* logger = pParam->logger;
    Queue* queue = pParam->queue;
    char logMsg[128];

    SHeader* hHeader;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 1024 * 48)) == -1)
        {
            sprintf(logMsg, "fail to receive from %s:%d", pParam->host, pParam->port);
            Log(logger, logMsg);
            close(pParam->clientSock);
            break;
        }
        
        hHeader = (SHeader*)buf;

        if (readSize == 0)
        {
            sprintf(logMsg, "disconnected %s:%d", pParam->host, pParam->port);
            Log(logger, logMsg);
            close(pParam->clientSock);
            break;
        }
        buf[readSize] = 0;

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

        uchar* receivedData = (uchar*)malloc(sizeof(uchar) * readSize);
        memcpy(receivedData, buf, readSize);
        pthread_mutex_lock(&queue->lock);
        Push(receivedData, queue);
        pthread_mutex_unlock(&queue->lock);
    }
    sprintf(logMsg, "kill receiver for %s:%d", pParam->host, pParam->port);
    Log(logger, logMsg);
}