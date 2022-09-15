#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include "hookerUtils.h"


static const char* defaultHostIp = "127.0.0.1";
static const unsigned short defaultHostPort = 4343;

static ssize_t (*RealSend)(int, const void*, size_t, int);
static int sockFd;
static struct sockaddr_in servAddr;
static socklen_t sockLen;
static struct SPrefixPkt prevPkt;
static size_t prePktSize;
static struct SPostfixPkt postPkt;
static size_t postPktSize;
static time_t loadTime;

// static int prevIdx;
// static float elapseAvg[TAGET_SECONDS];
// static int elapseCnt[TAGET_SECONDS];
// static ulong sendBytesAvg[TAGET_SECONDS];
// static int sendBytesCnt[TAGET_SECONDS];

static SAvgArray* avgArray[65536];

__attribute__((constructor))
static void InitializeHookingModule()
{
    // #00. Initialize variables that used in module
    loadTime = time(NULL);
    RealSend = (ssize_t (*)(int, const void*, size_t, int))dlsym(RTLD_NEXT, "send");
    sockFd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockFd == -1)
    {
        // TODO: handle error
        perror("agent");
        return ;
    }
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(defaultHostIp);
    servAddr.sin_port = htons(defaultHostPort);
    sockLen = sizeof(servAddr);

    // memset(elapseAvg, 0, sizeof(float) * TAGET_SECONDS);
    // memset(elapseCnt, 0, sizeof(int) * TAGET_SECONDS);
    // memset(sendBytesAvg, 0, sizeof(float) * TAGET_SECONDS);
    // memset(sendBytesCnt, 0, sizeof(int) * TAGET_SECONDS);

    // #01. Initialize packet data that befor sending real packet declared struct SPrevPkt
    prevPkt.pid = getpid();
    char buf[128];
    sprintf(buf, "/proc/%d/stat", prevPkt.pid);
    int fd = open(buf, O_RDONLY);
    if (fd == -1)
        strcpy(prevPkt.processName, "unknown");
    else
    {
        int readSize = read(fd, buf, 128);
        if (readSize < 1)
            strcpy(prevPkt.processName, "unknown");
        else
        {
            buf[readSize] = 0; 
            int i = 0, j = 0;
            while (buf[i++] != ' ');
            i++;
            for (; buf[i] != ')'; i++)
                prevPkt.processName[j++] = buf[i];            
        }
        close(fd);
    }
    strcpy(prevPkt.agentId, "udp-test");
    strcpy(prevPkt.hostIp, defaultHostIp);
    prevPkt.hostPort = defaultHostPort;
    prevPkt.packetNo = 0;
    prePktSize = sizeof(SPrefixPkt);

    // #02. Initialize packet data that after sending real packet declared struct SPostPkt
    strcpy(postPkt.agentId, prevPkt.agentId);
    strcpy(postPkt.processName, prevPkt.processName);
    postPkt.pid = prevPkt.pid;
    postPktSize = sizeof(SPostfixPkt);
}

void CalcAvgInSecondF(float* avg, int* avgCnt, float newValue)
{
    *avg = (*avg * (*avgCnt)) + newValue;
    (*avgCnt) += 1;
    *avg /= *avgCnt;
}

float CalcAvgInMinuteF(float* avgs)
{
    float result = 0.0;

    for (int i = 0; i < TAGET_SECONDS; i++)
        result += avgs[i];
    return result / TAGET_SECONDS;
}

void CalcAvgInSecondL(unsigned long* avg, int* avgCnt, unsigned long newValue)
{
    *avg = (*avg * (*avgCnt)) + newValue;
    (*avgCnt) += 1;
    *avg /= *avgCnt;
}

float CalcAvgInMinuteL(unsigned long* avgs)
{
    float result = 0.0;

    for (int i = 0; i < TAGET_SECONDS; i++)
        result += avgs[i];
    return (float)result / TAGET_SECONDS;
}

ssize_t send(int fd, const void* buf, size_t len, int flags)
{   
    if (avgArray[fd] == NULL)
    {
        avgArray[fd] = (SAvgArray*)malloc(sizeof(SAvgArray));
        avgArray[fd]->prevIdx = 0;
        memset(avgArray[fd]->elapseAvg, 0, sizeof(float) * TAGET_SECONDS);
        memset(avgArray[fd]->elapseCnt, 0, sizeof(int) * TAGET_SECONDS);
        memset(avgArray[fd]->sendBytesAvg, 0, sizeof(float) * TAGET_SECONDS);
        memset(avgArray[fd]->sendBytesCnt, 0, sizeof(int) * TAGET_SECONDS);
    }
    int idx = (time(NULL) - loadTime) % TAGET_SECONDS;
    if (avgArray[fd]->prevIdx != idx)
    {
        avgArray[fd]->elapseAvg[idx] = 0.0;
        avgArray[fd]->elapseCnt[idx] = 0;
        avgArray[fd]->sendBytesAvg[idx] = 0.0;
        avgArray[fd]->sendBytesCnt[idx] = 0;
        avgArray[fd]->prevIdx = idx;
    }

    // #00. send udp packet before sending real packet
    sendto(sockFd, &prevPkt, prePktSize, 0, (struct sockaddr*)&servAddr, sockLen);
    prevPkt.packetNo++;
    
    struct timeval tmpTime;
    gettimeofday(&tmpTime, NULL);
    ulong prevTime = tmpTime.tv_sec * 1000000 + tmpTime.tv_usec;
    
    // #01. send real packet
    ssize_t sendBytes = RealSend(fd, buf, len, flags);

    // #02. Get elapse time and that's average value
    gettimeofday(&tmpTime, NULL);
    ulong elapseTime = (tmpTime.tv_sec * 1000000 + tmpTime.tv_usec) - prevTime;
    if (postPkt.maxElapseTime < elapseTime)
        postPkt.maxElapseTime = elapseTime;
    
    CalcAvgInSecondF(&avgArray[fd]->elapseAvg[idx], &avgArray[fd]->elapseCnt[idx], elapseTime);
    postPkt.elapseTimeAvg = CalcAvgInMinuteF(avgArray[fd]->elapseAvg);
    postPkt.elapseTime = elapseTime;

    // #03. Get send bytes and that's average value
    if (postPkt.maxSendBytes < sendBytes)
        postPkt.maxSendBytes = sendBytes;
    CalcAvgInSecondL(&avgArray[fd]->sendBytesAvg[idx], &avgArray[fd]->sendBytesCnt[idx], sendBytes);
    postPkt.sendBytesAvg = CalcAvgInMinuteL(avgArray[fd]->sendBytesAvg);
    postPkt.sendBytes = sendBytes;
    postPkt.measurementTime = time(NULL);
    // // #04. send udp packet after sending real packet
    sendto(sockFd, &postPkt, postPktSize, 0, (struct sockaddr*)&servAddr, sockLen);
    
    return sendBytes;
}