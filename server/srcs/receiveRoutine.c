#include "receiveRoutine.h"
#include "pgWrapper.h"
#include "udpPacket.h"
#include "packets.h"
#include "logger.h"
#include "Queue.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <stdbool.h>

#define RECV_BUFFER_SIZE 1024 * 512

int udpSockFd;
extern const Logger* g_logger;
extern Queue* g_queue;
extern bool g_turnOff;
extern unsigned int g_clientCnt;
extern pthread_mutex_t g_clientCntLock;

int IsValidSignature(int signature)
{
    if (signature == SIGNATURE_CPU)
        return 1;
    else if (signature == SIGNATURE_AVG_CPU)
        return 1;
    else if (signature == SIGNATURE_NET)
        return 1;
    else if (signature == SIGNATURE_AVG_NET)
        return 1;
    else if (signature == SIGNATURE_MEM)
        return 1;
    else if (signature == SIGNATURE_AVG_MEM)
        return 1;
    else if (signature == SIGNATURE_PROC)
        return 1;
    else if (signature == SIGNATURE_DISK)
        return 1;
    return 0;
}

void* TcpReceiveRoutine(void* param)
{
    SReceiveParam* pParam = (SReceiveParam*)param;
    int readSize;
    int totalReadSize;
    char buf[RECV_BUFFER_SIZE] = { 0, };
    char logMsg[128];
    char* pb;
    int packetSize;

    SHeader* hHeader;
    while (1)
    {
        pb = buf;
        if (g_turnOff)
            break;
        
        if ((readSize = recv(pParam->clientSock, pb, sizeof(SHeader), 0)) == -1)
        {
            sprintf(logMsg, "Disconnect to agent %s:%d (Failed to receive packet)", pParam->host, pParam->port);
            Log(g_logger, LOG_ERROR, logMsg);
            break;
        }

        if (readSize == 0)
        {
            sprintf(logMsg, "Disconnected from %s:%d", pParam->host, pParam->port);
            Log(g_logger, LOG_INFO, logMsg);
            break;
        }
        
        hHeader = (SHeader*)pb;
        if (!IsValidSignature(hHeader->signature))
        {
            sprintf(logMsg, "Disconnect to agent %s:%d (Invalid packet signature)",
                pParam->host,
                pParam->port);
            Log(g_logger, LOG_FATAL, logMsg);
            break;
        }

        if (hHeader->signature == SIGNATURE_PROC)
            packetSize = hHeader->bodySize + sizeof(SHeader);
        else
            packetSize = (hHeader->bodyCount * hHeader->bodySize + sizeof(SHeader));
        totalReadSize = readSize;

        while (totalReadSize < packetSize)
        {
            if ((readSize = recv(pParam->clientSock, pb + totalReadSize, packetSize - totalReadSize, 0)) <= 0)
            {
                if (readSize == 0)
                {
                    sprintf(logMsg, "Disconnected from %s:%d", pParam->host, pParam->port);
                    Log(g_logger, LOG_INFO, logMsg);
                    break;
                }
                sprintf(logMsg, "Disconnect to agent %s:%d (Failed to receive packet)", pParam->host, pParam->port);
                Log(g_logger, LOG_ERROR, logMsg);
                break;
            }
            totalReadSize += readSize;
        }
        pb[totalReadSize] = 0;

        sprintf(logMsg, "Received packet from %s:%d %d B",
            pParam->host,
            pParam->port,
            packetSize);
        Log(g_logger, LOG_DEBUG, logMsg);
        

        uchar* receivedData = (uchar*)malloc(sizeof(uchar) * packetSize);
        memcpy(receivedData, pb, packetSize);
        pthread_mutex_lock(&g_queue->lock);
        Push(receivedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);
    }

    close(pParam->clientSock);
    sprintf(logMsg, "End receiver for %s:%d", pParam->host, pParam->port);
    Log(g_logger, LOG_INFO, logMsg);

    pthread_mutex_lock(&g_clientCntLock);
    g_clientCnt--;
    pthread_mutex_unlock(&g_clientCntLock);
}

void* UdpReceiveRoutine(void* param)
{
    SPgWrapper* db = (SPgWrapper*)param;

    udpSockFd = socket(PF_INET, SOCK_DGRAM, 0);
    if (udpSockFd < 0)
        return 0;
    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));

    udpAddr.sin_family = AF_INET;
    udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpAddr.sin_port = htons(4343);

    if (bind(udpSockFd, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0)
        return 0;
    
    int readSize;
    char buf[1024];
    char logMsg[256];
    struct sockaddr_in udpClientAddr;
    socklen_t len;
    SPrefixPkt* prevPkt;
    SPostfixPkt* postPkt;

    sprintf(logMsg, "Start UDP receiver");
    Log(g_logger, LOG_INFO, logMsg);
    char* strInsert = "INSERT INTO udp_informations (agent_id, measurement_time, process_name, pid, max_send_bytes, send_bytes_avg, max_elapse_time, elapse_time_avg) VALUES";
    char sql[512];
    while (1)
    {
        if (g_turnOff)
            break;

        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPrefixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            sprintf(logMsg, "Fail to receive UDP packet");
            Log(g_logger, LOG_ERROR, logMsg);
            continue;
        }
        buf[readSize];

        if (readSize != sizeof(SPrefixPkt))
        {
            // TODO: handle packet loss..
            continue;
        }
        
        prevPkt = (SPrefixPkt*)buf;
        // TODO: handle prevPkt...

        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPostfixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            sprintf(logMsg, "Fail to receive UDP packet");
            Log(g_logger, LOG_ERROR, logMsg);  
            continue;
        }
        buf[readSize];

        if (readSize != sizeof(SPostfixPkt))
        {
            // TODO: handle packet loss...
            continue;
        }

        postPkt = (SPostfixPkt*)buf;
        struct tm* ts;
        ts = localtime(&postPkt->measurementTime);
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %u, %u, %f, %lu, %f);",
            strInsert,
            postPkt->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            postPkt->processName,
            postPkt->pid,
            postPkt->maxSendBytes,
            postPkt->sendBytesAvg,
            postPkt->maxElapseTime,
            postPkt->elapseTimeAvg);

        if (Query(db, sql) == -1)
        {
            sprintf(sql, "Failed to store in DB: UDP");
            Log(g_logger, LOG_ERROR, sql);
            continue;
        }
    }

    sprintf(logMsg, "End UDP receiver");
    Log(g_logger, LOG_INFO, logMsg);
}
