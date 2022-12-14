#include "receiveRoutine.h"
#include "pgWrapper.h"
#include "hookerUtils.h"
#include "packets.h"
#include "logger.h"
#include "queue.h"
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
extern pthread_mutex_t g_workerLock;

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
    char* pb;
    int packetSize;

    LOG_INFO(g_logger, "Start communication with %s:%d", pParam->host, pParam->port);

    SHeader* hHeader;
    while (1)
    {
        pb = buf;
        if (g_turnOff)
            break;
        
        if ((readSize = recv(pParam->clientSock, pb, sizeof(SHeader), 0)) == -1)
        {
            LOG_ERROR(g_logger, "Disconnect to agent %s:%d (Failed to receive packet)", pParam->host, pParam->port);
            break;
        }

        if (readSize == 0)
        {
            LOG_INFO(g_logger, "Disconnected from %s:%d", pParam->host, pParam->port);
            break;
        }
        
        hHeader = (SHeader*)pb;
        if (!IsValidSignature(hHeader->signature))
        {
            LOG_FATAL(g_logger,"Disconnect to agent %s:%d (Invalid packet signature)",
                pParam->host,
                pParam->port);
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
                    LOG_INFO(g_logger, "Disconnected from %s:%d", pParam->host, pParam->port);
                    break;
                }
                LOG_ERROR(g_logger, "Disconnect to agent %s:%d (Failed to receive packet)", pParam->host, pParam->port);
                break;
            }
            totalReadSize += readSize;
        }
        pb[totalReadSize] = 0;

        LOG_DEBUG(g_logger, "Received packet from %s:%d %d B",
            pParam->host,
            pParam->port,
            packetSize);

        uchar* receivedData = (uchar*)malloc(sizeof(uchar) * packetSize);
        memcpy(receivedData, pb, packetSize);
        pthread_mutex_lock(&g_queue->lock);
        Push(receivedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);
    }

    close(pParam->clientSock);
    LOG_INFO(g_logger, "End communication with %s:%d", pParam->host, pParam->port);

    pthread_mutex_lock(&g_workerLock);
    g_clientCnt--;
    pthread_mutex_unlock(&g_workerLock);
}

void* UdpReceiveRoutine(void* param)
{
    SPgWrapper* db = NewPgWrapper("dbname = postgres port = 5442");

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
    
    int sqlCnt = 0;
    int readSize;
    char buf[1024];
    struct sockaddr_in udpClientAddr;
    socklen_t len;
    SPrefixPkt* prevPkt;
    SPostfixPkt* postPkt;

    LOG_INFO(g_logger, "Start UDP receiver");
    char* strInsert = "INSERT INTO udp_informations (agent_id, measurement_time, process_name, pid, send_bytes, max_send_bytes, send_bytes_avg, elapse_time, max_elapse_time, elapse_time_avg) VALUES";
    char sql[512];
    Query(db, "BEGIN");
    while (1)
    {
        if (g_turnOff)
            break;

        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPrefixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            LOG_ERROR(g_logger, "Fail to receive UDP packet");
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
            LOG_ERROR(g_logger, "Fail to receive UDP packet");
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
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %u, %u, %u, %f, %lu, %lu, %f);",
            strInsert,
            postPkt->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            postPkt->processName,
            postPkt->pid,
            postPkt->sendBytes,
            postPkt->maxSendBytes,
            postPkt->sendBytesAvg,
            postPkt->elapseTime,
            postPkt->maxElapseTime,
            postPkt->elapseTimeAvg);

        if (sqlCnt >= 128)
        {
            Query(db, "COMMIT");
            sqlCnt = 0;
            Query(db, "BEGIN");
        }

        if (Query(db, sql) == -1)
        {
            LOG_ERROR(g_logger, "Failed to store in DB: UDP");
            continue;
        }
        sqlCnt++;
    }

    LOG_INFO(g_logger, "End UDP receiver");
}
