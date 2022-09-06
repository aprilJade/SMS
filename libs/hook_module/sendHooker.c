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
#include <sys/time.h>
#include <assert.h>
#include "udpPacket.h"

#define TAGET_SECONDS 60

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
static float elapseAvg[TAGET_SECONDS];
static int elapseCnt[TAGET_SECONDS];
static ulong elapseTime;
static ulong prevTime;
static struct timeval tmpTime;
static int prevIdx;
static ulong sendBytesAvg[TAGET_SECONDS];
static int sendBytesCnt[TAGET_SECONDS];
static ssize_t sendBytes;

static void InitializeUdpPacket(struct SPrefixPkt* prevPkt, struct SPostfixPkt* postPkt)
{
    /* Initialize Prefix Packet */
    prevPkt->pid = getpid();
    char buf[128];
    sprintf(buf, "/proc/%d/stat", prevPkt->pid);
    int fd = open(buf, O_RDONLY);
    if (fd == -1)
        strcpy(prevPkt->processName, "unknown");
    else
    {
        int readSize = read(fd, buf, 128);
        if (readSize < 1)
            strcpy(prevPkt->processName, "unknown");
        else
        {
            buf[readSize] = 0; 
            int i = 0, j = 0;
            while (buf[i++] != ' ');
            i++;
            for (; buf[i] != ')'; i++)
                prevPkt->processName[j++] = buf[i];            
        }
        close(fd);
    }
    strcpy(prevPkt->agentId, "udp-test");
    strcpy(prevPkt->hostIp, defaultHostIp);
    prevPkt->hostPort = defaultHostPort;
    prevPkt->packetNo = 0;
    prePktSize = sizeof(SPrefixPkt);

    /* Initialize Postfix Packet */
    strcpy(postPkt->agentId, prevPkt->agentId);
    strcpy(postPkt->processName, prevPkt->processName);
    postPkt->pid = prevPkt->pid;
    postPktSize = sizeof(SPostfixPkt);
}

__attribute__((constructor))
static void InitializeHookingModule()
{
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

    memset(elapseAvg, 0, sizeof(float) * TAGET_SECONDS);
    memset(elapseCnt, 0, sizeof(int) * TAGET_SECONDS);
    memset(sendBytesAvg, 0, sizeof(float) * TAGET_SECONDS);
    memset(sendBytesCnt, 0, sizeof(int) * TAGET_SECONDS);
    
    InitializeUdpPacket(&prevPkt, &postPkt);
}


ssize_t send(int fd, const void* buf, size_t len, int flags)
{   
    int idx = (time(NULL) - loadTime) % TAGET_SECONDS;
    if (prevIdx != idx)
    {
        elapseAvg[idx] = 0.0;
        elapseCnt[idx] = 0;
        sendBytesAvg[idx] = 0.0;
        sendBytesCnt[idx] = 0;
        prevIdx = idx;
    }

    // #0. send udp packet before send real packet
    sendto(sockFd, &prevPkt, prePktSize, 0, (struct sockaddr*)&servAddr, sockLen);
    prevPkt.packetNo++;
    
    gettimeofday(&tmpTime, NULL);
    prevTime = tmpTime.tv_sec * 1000000 + tmpTime.tv_usec;
    
    // #1. send real packet
    sendBytes = RealSend(fd, buf, len, flags);

    // #2. Get elapse time and that's average value
    gettimeofday(&tmpTime, NULL);
    elapseTime = (tmpTime.tv_sec * 1000000 + tmpTime.tv_usec) - prevTime;
    if (postPkt.maxElapseTime < elapseTime)
        postPkt.maxElapseTime = elapseTime;
    elapseCnt[idx]++;
    elapseAvg[idx] = ((elapseAvg[idx] * (elapseCnt[idx] - 1)) + elapseTime) / elapseCnt[idx];
    postPkt.elapseTimeAvg = 0;
    for (int i = 0; i < TAGET_SECONDS; i++)
        postPkt.elapseTimeAvg += elapseAvg[i];
    postPkt.elapseTimeAvg /= TAGET_SECONDS;

    // #3. Get send bytes and that's average value
    if (postPkt.maxSendBytes < sendBytes)
        postPkt.maxSendBytes = sendBytes;
    sendBytesCnt[idx]++;
    sendBytesAvg[idx] = ((sendBytesAvg[idx] * (sendBytesCnt[idx] - 1)) + sendBytes) / sendBytesCnt[idx];
    postPkt.sendBytesAvg = 0;
    for (int i = 0; i < TAGET_SECONDS; i++)
        postPkt.sendBytesAvg += sendBytesAvg[i];
    postPkt.sendBytesAvg /= TAGET_SECONDS;
    postPkt.measurementTime = time(NULL);
    // #4. send udp packet after send real packet
    sendto(sockFd, &postPkt, postPktSize, 0, (struct sockaddr*)&servAddr, sockLen);
    
    return sendBytes;
}