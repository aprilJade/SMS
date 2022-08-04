#include "collectRoutine.h"
#include "collector.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "tcpCtrl.h"

#define NO_SLEEP 0
#define PRINT_CPU 0
#define PRINT_MEM 0
#define PRINT_NET 0
#define PRINT_PROC 0


#define HOST "127.0.0.1"
#define PORT 4243

void* cpuInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
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
    SInitialPacket initPacket;
    memcpy(&initPacket.signature, SIGNATURE_CPU, 4);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }
    memset(&packet, 0, sizeof(SCpuInfoPacket));
    memcpy(&packet.signature, SIGNATURE_CPU, 4);
    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        collectCpuInfo(toMs, buf, &packet);
#if PRINT_CPU
        // TODO: Convert printf to log
        printf("<CPU information as OS resources>\n");
        printf("CPU running time (user mode): %d ms\n", packet.usrCpuRunTime);
        printf("CPU running time (system mode): %d ms\n", packet.sysCpuRunTime);
        printf("CPU idle time: %d ms\n", packet.idleTime);
        printf("CPU I/O wait time: %d ms\n", packet.waitTime);
        printf("Collect starting time: %lld ms\n", packet.collectTime);
        printf("Send cpu info packet.\n");
#endif
        if (write(sockFd, &packet, sizeof(SCpuInfoPacket)) == -1)
        {
            // TODO: handle error
            perror("agent");
            return 0;
        }
        sendPacketCount++;
        // TODO: Caching.... packet data
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
#if !NO_SLEEP
        printf("cpu packet sended count: %ld\n", sendPacketCount);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

void* memInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulonglong prevTime, postTime, elapseTime;
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
    SInitialPacket initPacket;
    memcpy(&initPacket.signature, SIGNATURE_MEM, 4);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }
    memset(&packet, 0, sizeof(SMemInfoPacket));
    memcpy(&packet.signature, SIGNATURE_MEM, 4);
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        collectMemInfo(buf, &packet);
#if PRINT_MEM
        printf("<Memory information>\n");
        printf("Total memory: %d kB\n", packet.memTotal);
        printf("Free memory: %d kB\n", packet.memFree);
        printf("Used memory: %d kB\n", packet.memUsed);
        printf("Available memory: %d kB\n", packet.memAvail);
        printf("Total swap: %d kB\n", packet.swapTotal);
        printf("Free swap: %d kB\n", packet.swapFree);
        printf("Collecting start time: %lld\n\n", packet.collectTime);
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
        // TODO: Caching.... packet data
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
#if !NO_SLEEP
        printf("mem packet sended count: %ld\n", sendPacketCount);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

void* netInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulonglong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SNetInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }
    SInitialPacket initPacket;
    memcpy(&initPacket.signature, SIGNATURE_NET, 4);
    if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return 0;
    }
    memset(&packet, 0, sizeof(SNetInfoPacket));
    memcpy(&packet.signature, SIGNATURE_NET, 4);
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        collectNetInfo(buf, &packet);
#if PRINT_NET
        printf("Collected net info packet\n\
                Collect Time: %lld ms\n\
                Network Interface: %s\n\
                Receive Bytes: %lld B\n\
                Receive Packet Count: %d\n\
                Send Bytes: %lld B\n\
                Send Packet Count: %d\n",
                packet.collectTime,
                packet.netIfName,
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
        // TODO: Caching.... packet data
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
#if !NO_SLEEP
        printf("Net packet sended count: %ld\n", sendPacketCount);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}

void* procInfoRoutine(void* param)
{
    size_t sendPacketCount = 0;
    ulonglong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SProcInfoPacket packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    int sockFd = ConnectToServer(HOST, PORT);
    if (sockFd == -1)
    {
        // TODO: handle error
        printf("Fail to connect to server\n");
        return 0;
    }
    SInitialPacket initPacket;
    memcpy(&initPacket.signature, SIGNATURE_PROC, 4);
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
    memcpy(&packet.signature, SIGNATURE_PROC, 4);
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
                    collectProcInfo(path, buf, &packet);
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
                printf("Send process info packet: %d\n", packet.pid);
                printf("Collect process info\n\
                        Collect Time: %lld\n\
                        PID: %d\n\
                        PPID: %d\n\
                        Running Time (user): %d\n\
                        Running Time (system): %d\n\
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
                // TODO: Caching.... packet data
                gettimeofday(&timeVal, NULL);
                postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
                elapseTime = postTime - prevTime;
            }
        }
#if !NO_SLEEP
        printf("Proc packet sended count: %ld\n", sendPacketCount);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
#endif
        // TODO: Check TCP connection
        // If disconnected, reconnect!
    }
}