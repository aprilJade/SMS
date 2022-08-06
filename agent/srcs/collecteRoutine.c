#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "collectRoutine.h"
#include "collector.h"
#include "tcpCtrl.h"

#define NO_SLEEP 0
#define PRINT_CPU 0
#define PRINT_MEM 0
#define PRINT_NET 0
#define PRINT_PROC 0

#define HOST "127.0.0.1"
#define PORT 4243

SRoutineParam* GenRoutineParam(int collectPeriod)
{
    SRoutineParam* ret = (SRoutineParam*)malloc(sizeof(SRoutineParam));
    if (ret->collectPeriod < MIN_SLEEP_MS)
    {
        printf("Minimum collect period is 500 ms\n\
                But you input %d ms. This is not allowed.\n\
                Set collect period to 500 ms\n", ret->collectPeriod);
        ret->collectPeriod = MIN_SLEEP_MS;
    }
    else
    {
        ret->collectPeriod = collectPeriod;
    }
    return ret;
}

void* CpuInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulong prevTime, postTime, elapseTime;
	long toMs = 1000 / sysconf(_SC_CLK_TCK);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SCpuInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;

    // TODO: Change to Log
    printf("Collect CPU information every %d ms.\n", pParam->collectPeriod);

    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }

    SInitialPacket initPacket;
    GenerateInitialCpuPacket(&initPacket, pParam);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }

    memset(&packet, 0, sizeof(SCpuInfoPacket));
    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        CollectCpuInfo(toMs, buf, &packet);
#if PRINT_CPU
        // TODO: Convert printf to log
        printf("<CPU information as OS resources>\n");
        printf("CPU running time (user mode): %ld ms\n", packet.usrCpuRunTime);
        printf("CPU running time (system mode): %ld ms\n", packet.sysCpuRunTime);
        printf("CPU idle time: %ld ms\n", packet.idleTime);
        printf("CPU I/O wait time: %ld ms\n", packet.waitTime);
        printf("Collect starting time: %ld ms\n", packet.collectTime);
        printf("Send cpu info packet.\n");
#endif
        if (write(sockFd, &packet, sizeof(SCpuInfoPacket)) == -1)
        {
            // TODO: handle error
            perror("agent");
            return 0;
        }
        sendPacketCount++;
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
#if !NO_SLEEP
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

void* MemInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SMemInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;

    // TODO: Change to Log
    printf("Collect memory information every %d ms.\n", pParam->collectPeriod);

    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }

    SInitialPacket initPacket;
    GenerateInitialMemPacket(&initPacket, pParam);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }

    memset(&packet, 0, sizeof(SMemInfoPacket));
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        CollectMemInfo(buf, &packet);
#if PRINT_MEM
        printf("<Memory information>\n");
        printf("Free memory: %ld kB\n", packet.memFree);
        printf("Used memory: %ld kB\n", packet.memUsed);
        printf("Available memory: %ld kB\n", packet.memAvail);
        printf("Free swap: %ld kB\n", packet.swapFree);
        printf("Collecting start time: %ld\n\n", packet.collectTime);
#endif
        if (write(sockFd, &packet, sizeof(SMemInfoPacket)) == -1)
        {
            // TODO: handle error
            perror("agent");
            return 0;
        }
        sendPacketCount++;
#if PRINT_MEM
        printf("Send memory info packet.\n");
#endif
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
#if !NO_SLEEP
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

// TODO: handle multiple network interface
void* NetInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SNetInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;

    // TODO: Change to Log
    printf("Collect network information every %d ms.\n", pParam->collectPeriod);

    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }

    SInitialPacket initPacket;
    GenerateInitialNetPacket(&initPacket, pParam);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }

    memset(&packet, 0, sizeof(SNetInfoPacket));
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        CollectNetInfo(buf, &packet);
#if PRINT_NET
        printf("Collected net info packet\n\
                network interface name: %s\n\
                Collect Time: %ld ms\n\
                Receive Bytes: %ld kB\n\
                Receive Packet Count: %ld\n\
                Send Bytes: %ld kB\n\
                Send Packet Count: %ld\n",
                initPacket.netIfName,
                packet.collectTime,
                packet.recvBytes,
                packet.recvPackets,
                packet.sendBytes,
                packet.sendPackets);
#endif
        if (write(sockFd, &packet, sizeof(SNetInfoPacket)) == -1)
        {
            // TODO: handle error
            perror("agent");
            return 0;
        }
        sendPacketCount++;
#if PRINT_NET
        printf("Send network info packet.\n");
#endif
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
#if !NO_SLEEP
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

void* ProcInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SProcInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;

    // TODO: Change to Log
    printf("Collect process information every %d ms.\n", pParam->collectPeriod);

    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }

    SInitialPacket initPacket;
    GenerateInitialProcPacket(&initPacket, pParam);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }

    DIR* dir;
    struct dirent* entry;
    char path[260];
    memset(&packet, 0, sizeof(SProcInfoPacket));
    while(1)
    {
        dir = opendir("/proc");
        if (dir == NULL)
            return 0;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_DIR)
            {
                if (atoi(entry->d_name) < 1)
                    continue;
                snprintf(path, 262, "/proc/%s", entry->d_name);
                if (gettimeofday(&timeVal, NULL) == -1)
                {
                    // TODO: handling error
                    perror("agent");
                    return 0;
                }    
                prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
                packet.collectTime = prevTime / 1000;
                if (access(path, F_OK) == 0)
                    CollectProcInfo(path, buf, &packet);
                else
                    continue;
                if (write(sockFd, &packet, sizeof(SProcInfoPacket)) == -1)
                {
                    // TODO: handle error
                    perror("agent");
                    return 0;
                }
                sendPacketCount++;
#if PRINT_PROC
                printf("Send process info packet: %u\n", packet.pid);
                printf("Collect process info\n\
                        Collect Time: %ld\n\
                        PID: %u\n\
                        PPID: %u\n\
                        Running Time (user): %ld\n\
                        Running Time (system): %ld\n\
                        User name: %s\n\
                        Process Name: %s\n\
                        State: %c\n",
                        packet.collectTime,
                        packet.pid,
                        packet.ppid,
                        packet.utime,
                        packet.stime,
                        packet.userName,
                        packet.procName,
                        packet.state);
#endif
                gettimeofday(&timeVal, NULL);
                postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
                elapseTime = postTime - prevTime;
            }
        }
#if !NO_SLEEP
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}