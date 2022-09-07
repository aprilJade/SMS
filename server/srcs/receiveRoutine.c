#include "receiveRoutine.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <stdbool.h>

#define RECV_BUFFER_SIZE 1024 * 512

extern const Logger* g_logger;
extern Queue* g_queue;
extern bool g_turnOff;
extern unsigned int g_clientCnt;
extern pthread_mutex_t g_clientCntLock;

int IsValidSignature(int signature)
{
    if (signature == SIGNATURE_CPU)
        return 1;
    else if (signature == SIGNATURE_MEM)
        return 1;
    else if (signature == SIGNATURE_NET)
        return 1;
    else if (signature == SIGNATURE_PROC)
        return 1;
    else if (signature == SIGNATURE_DISK)
        return 1;
    else if (signature == SIGNATURE_AVG_CPU)
        return 1;
    else if (signature == SIGNATURE_AVG_MEM)
        return 1;
    else if (signature == SIGNATURE_AVG_NET)
        return 1;
    return 0;
}

void* ReceiveRoutine(void* param)
{
    SReceiveParam* pParam = (SReceiveParam*)param;
    int readSize;
    char buf[RECV_BUFFER_SIZE] = { 0, };
    char logMsg[128];
    char* pb;
    int packetSize;

    SHeader* hHeader;
    while (1)
    {
        if (g_turnOff)
            break;
        if ((readSize = recv(pParam->clientSock, buf, RECV_BUFFER_SIZE, 0)) == -1)
        {
            sprintf(logMsg, "Disconnect to agent %s:%d (Failed to receive packet)", pParam->host, pParam->port);
            Log(g_logger, LOG_ERROR, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb = buf;

        if (readSize == 0)
        {
            sprintf(logMsg, "Disconnected from %s:%d", pParam->host, pParam->port);
            Log(g_logger, LOG_INFO, logMsg);
            close(pParam->clientSock);
            break;
        }
        pb[readSize] = 0;

        while (readSize > 0)
        {
            //printf("%d\n", readSize);
            hHeader = (SHeader*)pb;
            if (!IsValidSignature(hHeader->signature))
            {
                sprintf(logMsg, "Disconnect to agent %s:%d (Invalid packet signature)",
                    pParam->host,
                    pParam->port);
                Log(g_logger, LOG_FATAL, logMsg);
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
            Log(g_logger, LOG_DEBUG, logMsg);

            uchar* receivedData = (uchar*)malloc(sizeof(uchar) * packetSize);
            memcpy(receivedData, pb, packetSize);
            readSize -= packetSize;
            pb += packetSize;
            pthread_mutex_lock(&g_queue->lock);
            Push(receivedData, g_queue);
            pthread_mutex_unlock(&g_queue->lock);
        }
    }
    close(pParam->clientSock);
    sprintf(logMsg, "End receiver for %s:%d", pParam->host, pParam->port);
    Log(g_logger, LOG_INFO, logMsg);

    pthread_mutex_lock(&g_clientCntLock);
    g_clientCnt--;
    pthread_mutex_lock(&g_clientCntLock);
}
