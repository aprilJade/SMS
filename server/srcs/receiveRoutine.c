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
            sprintf(logMsg, "Failed to receive packet from %s:%d", pParam->host, pParam->port);
            Log(logger, LOG_ERROR, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb = buf;

        if (readSize == 0)
        {
            sprintf(logMsg, "Disconnected from %s:%d", pParam->host, pParam->port);
            Log(logger, LOG_INFO, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb[readSize] = 0;


        while (readSize > 0)
        {
            hHeader = (SHeader*)pb;
            if (!IsValidSignature(hHeader->signature))
            {
                sprintf(logMsg, "Invalid packet signature from %s:%d",
                    pParam->host,
                    pParam->port);
                Log(logger, LOG_FATAL, logMsg);
                close(pParam->clientSock);
                break;
            }

            if (hHeader->signature == SIGNATURE_PROC)
                packetSize = hHeader->bodySize + sizeof(SHeader);
            else
                packetSize = (hHeader->bodyCount * hHeader->bodySize + sizeof(SHeader));
            
            sprintf(logMsg, "Received packet from %s:%d %d B",
                pParam->host,
                pParam->port,
                packetSize);
            Log(logger, LOG_DEBUG, logMsg);

            uchar* receivedData = (uchar*)malloc(sizeof(uchar) * packetSize);
            memcpy(receivedData, pb, packetSize);
            readSize -= packetSize;
            pb += packetSize;
            pthread_mutex_lock(&queue->lock);
            Push(receivedData, queue);
            pthread_mutex_unlock(&queue->lock);
        }
    }
    sprintf(logMsg, "End receiver for %s:%d", pParam->host, pParam->port);
    Log(logger, LOG_INFO, logMsg);
}