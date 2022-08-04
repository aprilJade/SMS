#include "collectRoutine.h"
#include "collector.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include "tcpCtrl.h"

#define PRINT 1
#define SIGNATURE_CPU "SMSc"
#define SIGNATURE_MEM "SMSm"
#define SIGNATURE_NET "SMSn"
#define SIGNATURE_PROC "SMSp"

#define HOST "127.0.0.1"
#define PORT 4243

void* cpuInfoRoutine(void* param)
{
    // TODO: handle exit condition
    ulonglong prevTime, postTime, elapseTime;
	long toMs = 1000 / sysconf(_SC_CLK_TCK);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SCpuInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }
    while (1)
    {
        memset(&packet, 0, sizeof(SCpuInfoPacket));
        if (gettimeofday(&timeVal, NULL) == -1)
        {
            // TODO: handling error
            perror("agent");
            return 0;
        }    
        prevTime = timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000;
        packet.collectTime = prevTime;
        memcpy(&packet.signature, SIGNATURE_CPU, 4);
        collectCpuInfo(toMs, buf, &packet);
#if PRINT
        // TODO: Convert printf to log
        printf("<CPU information as OS resources>\n");
        printf("CPU running time (user mode): %d ms\n", packet.usrCpuRunTime);
        printf("CPU running time (system mode): %d ms\n", packet.sysCpuRunTime);
        printf("CPU idle time: %d ms\n", packet.idleTime);
        printf("CPU I/O wait time: %d ms\n", packet.waitTime);
        printf("Collect starting time: %lld ms\n", packet.collectTime);
#endif
        if (write(sockFd, &packet, sizeof(SCpuInfoPacket)) == -1)
        {
            // TODO: handle error
            perror("agent");
            return 0;
        }
        // TODO: Caching.... packet data
        if (gettimeofday(&timeVal, NULL) == -1)
        {
            // TODO: handling error
            perror("agent");
            return 0;
        }    
        postTime = timeVal.tv_sec * 1000  + timeVal.tv_usec / 1000;
        elapseTime = postTime - prevTime;
        usleep(pParam->collectPeriod * 1000 - elapseTime);
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

void* memInfoRoutine(void* param)
{
    ulonglong prevTime, postTime, elapseTime;
	long toMs = 1000 / sysconf(_SC_CLK_TCK);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SMemInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }

    while(1)
    {
        memset(&packet, 0, sizeof(SMemInfoPacket));
        if (gettimeofday(&timeVal, NULL) == -1)
        {
            // TODO: handling error
            perror("agent");
            return 0;
        }    
        prevTime = timeVal.tv_sec * 1000 + timeVal.tv_usec / 1000;
        packet.collectTime = prevTime;
        memcpy(&packet.signature, SIGNATURE_MEM, 4);
        collectMemInfo(buf, &packet);
#if PRINT
        printf("<Memory information>\n");
        printf("Total memory: %d kB\n", packet.memTotal);
        printf("Free memory: %d kB\n", packet.memFree);
        printf("Used memory: %d kB\n", packet.memUsed);
        printf("Available memory: %d kB\n", packet.memAvail);
        printf("Total swap: %d kB\n", packet.swapTotal);
        printf("Free swap: %d kB\n", packet.swapFree);
        printf("Collecting start time: %lld\n\n", packet.collectTime);
#endif
        if (write(sockFd, &packet, sizeof(SCpuInfoPacket)) == -1)
        {
            // TODO: handle error
            perror("agent");
            return 0;
        }
        // TODO: Caching.... packet data
        if (gettimeofday(&timeVal, NULL) == -1)
        {
            // TODO: handling error
            perror("agent");
            return 0;
        }    
        postTime = timeVal.tv_sec * 1000  + timeVal.tv_usec / 1000;
        elapseTime = postTime - prevTime;
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}