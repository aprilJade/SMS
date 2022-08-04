#include "collectRoutine.h"
#include "collector.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include "tcpCtrl.h"

#define PRINT 1
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
    int sockFd = ConnectToServer(HOST, PORT);
    SRoutineParam* pParam = (SRoutineParam*)param;
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
        memcpy(&packet.signature, "SMSc", 4);
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