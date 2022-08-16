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
            sprintf(logMsg, "ERR: FATAL: Failed to receive packet from %s:%d", pParam->host, pParam->port);
            Log(logger, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb = buf;

        if (readSize == 0)
        {
            sprintf(logMsg, "ERR: Disconnected from %s:%d", pParam->host, pParam->port);
            Log(logger, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb[readSize] = 0;


        while (readSize > 0)
        {
            hHeader = (SHeader*)pb;
            if (!IsValidSignature(hHeader->signature))
            {
                char strSig[5];
                memcpy(strSig, &hHeader->signature, 4);
                strSig[4] = 0;
                sprintf(logMsg, "ERR: FATAL: Invalid packet signature from %s:%d %s",
                    pParam->host,
                    pParam->port,
                    strSig);
                Log(logger, logMsg);
                close(pParam->clientSock);
                break;
            }


            packetSize = (hHeader->bodyCount * hHeader->bodySize + sizeof(SHeader));
            
            sprintf(logMsg, "INFO: Received packet from %s:%d %d B",
                pParam->host,
                pParam->port,
                packetSize);
            Log(logger, logMsg);
            
            uchar* receivedData = (uchar*)malloc(sizeof(uchar) * packetSize);
            memcpy(receivedData, pb, packetSize);
            readSize -= packetSize;
            pb += packetSize;
            pthread_mutex_lock(&queue->lock);
            Push(receivedData, queue);
            pthread_mutex_unlock(&queue->lock);

        }
    }
    sprintf(logMsg, "INFO: Close receiver for %s:%d", pParam->host, pParam->port);
    Log(logger, logMsg);
}