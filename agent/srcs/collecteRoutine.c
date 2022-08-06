#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "collectRoutine.h"
#include "collector.h"
#include "tcpCtrl.h"

#define RECONNECT_PERIOD 6    // seconds
#define RECONNECT_MAX_TRY 100

#define NO_SLEEP 0
#define PRINT_CPU 0
#define PRINT_MEM 0
#define PRINT_NET 0
#define PRINT_PROC 0

#define HOST "127.0.0.1"
#define PORT 4244

SRoutineParam* GenRoutineParam(int collectPeriod, int collectorID)
{
    SRoutineParam* ret = (SRoutineParam*)malloc(sizeof(SRoutineParam));
    if (collectPeriod < MIN_SLEEP_MS)
    {
        printf("Minimum collect period is 500 ms\n\
                But you input %d ms. This is not allowed.\n\
                Set collect period to 500 ms\n", collectPeriod);
        ret->collectPeriod = MIN_SLEEP_MS;
    }
    else
        ret->collectPeriod = collectPeriod;
    ret->queue = NewQueue();
    ret->collectorID = collectorID;
    return ret;
}
/*
void* CpuInfoRoutine(void* param)
{
    int i = 0;
    ulong prevTime, postTime, elapseTime;
	long toMs = 1000 / sysconf(_SC_CLK_TCK);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SCpuInfoPacket packet;
    memset(&packet, 0, sizeof(SCpuInfoPacket));
    SRoutineParam* pParam = (SRoutineParam*)param;
    int ret;

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
    while ((ret = write(sockFd, &initPacket, sizeof(SInitialPacket))) == -1)
    {
        close(sockFd);
        fprintf(stderr, "Server die\n");
        i = 0;
        do 
        {
            fprintf(stderr, "try to reconnect...(%d)\n", i++);
            sleep(2);
        } while ((sockFd = ConnectToServer(HOST, PORT)) == -1);
        printf("Reconnected to server! Try to resume CPU information collecting.\n");
    }

    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        packet.collectTime = prevTime / 1000;
        CollectCpuInfo(toMs, buf, &packet);
        if (write(sockFd, &packet, sizeof(SCpuInfoPacket)) == -1)
        {
            close(sockFd);
            fprintf(stderr, "Server die\n");
            i = 0;
            do 
            {
                fprintf(stderr, "try to reconnect...(%d)\n", i++);
                sleep(2);
            } while ((sockFd = ConnectToServer(HOST, PORT)) == -1);
            printf("Reconnected to server! Try to resume CPU information collecting.\n");
            write(sockFd, &initPacket, sizeof(SInitialPacket));
            continue;
        }
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}
*/
void* CpuInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
	long toMs = 1000 / sysconf(_SC_CLK_TCK);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SCpuInfoPacket* packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;

    printf("CPU Collector: Start to collect CPU information every %d ms.\n", pParam->collectPeriod);

    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        packet = (SCpuInfoPacket*)malloc(sizeof(SCpuInfoPacket));
        if (packet == NULL)
        {
            // TODO: handling malloc fail
            usleep(5000);
            continue;
        }
        memset(packet, 0, sizeof(SCpuInfoPacket));
        packet->collectTime = prevTime / 1000;

        CollectCpuInfo(toMs, buf, packet);

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

        pthread_mutex_lock(&queue->lock);
        // TODO: Handling Queue is full...more graceful
        while (IsFull(queue))
            usleep(500);
        Push(packet, queue);
        pthread_mutex_unlock(&queue->lock);
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}

void* MemInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SMemInfoPacket* packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;

    // TODO: Change to Log
    printf("Memory Collector: Start to collect memory information every %d ms.\n", pParam->collectPeriod);

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        
        packet = (SMemInfoPacket*)malloc(sizeof(SMemInfoPacket));
        if (packet == NULL)
        {
            // TODO: handling malloc fail
            usleep(5000);
            continue;
        }
        memset(packet, 0, sizeof(SMemInfoPacket));
        packet->collectTime = prevTime / 1000;

        CollectMemInfo(buf, &packet);

#if PRINT_MEM
        printf("<Memory information>\n");
        printf("Free memory: %ld kB\n", packet.memFree);
        printf("Used memory: %ld kB\n", packet.memUsed);
        printf("Available memory: %ld kB\n", packet.memAvail);
        printf("Free swap: %ld kB\n", packet.swapFree);
        printf("Collecting start time: %ld\n\n", packet.collectTime);
#endif

        pthread_mutex_lock(&queue->lock);
        // TODO: Handling Queue is full...more graceful
        while (IsFull(queue))
            usleep(500);
        Push(packet, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        usleep(pParam->collectPeriod * 1000 - elapseTime);
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

void* SendRoutine(void* param)
{
    SRoutineParam* pParam = (SRoutineParam*)param;
    void (*GenInitPacket)(SInitialPacket*, SRoutineParam*);
    Queue* queue = pParam->queue;
    int packetSize = 0;
    switch (pParam->collectorID)
    {
        case CPU:
            GenInitPacket = GenerateInitialCpuPacket;
            packetSize = sizeof(SCpuInfoPacket);
            break;
        case MEMORY:
            GenInitPacket = GenerateInitialMemPacket;
            packetSize = sizeof(SMemInfoPacket);
            break;
        case NETWORK:
            GenInitPacket = GenerateInitialNetPacket;
            packetSize = sizeof(SNetInfoPacket);
            break;
        case PROCESS:
            GenInitPacket = GenerateInitialProcPacket;
            packetSize = sizeof(SProcInfoPacket);
            break;
    }

    SInitialPacket initPacket;
    void* packet = NULL;
    int sockFd, reconnectTryCount = 0;
    while(1)
    {
        pritnf("CPU sender: Try to connect to server.\n");
        while ((sockFd = ConnectToServer(HOST, PORT)) == -1)
        {
            fprintf(stderr, "CPU sender: Fail to connect to server. try to reconnect...(%d)\n", reconnectTryCount++);
            sleep(RECONNECT_PERIOD);
            continue;
        }

        printf("CPU sender: success to connect to server.\n");

        GenInitPacket(&initPacket, pParam);
        if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
        {
            close(sockFd);
            fprintf(stderr, "CPU sender: Server has no reponse. Stop collecting CPU information.\n");
            continue;
        }
        printf("CPU sender: Start send information every %d ms.\n", pParam->collectPeriod);

        while (1)
        {
            if (packet == NULL)
            {
                pthread_mutex_lock(&queue->lock);
                packet = Pop(queue);
                if (packet == NULL)
                {
                    pthread_mutex_unlock(&queue->lock);
                    usleep(pParam->collectPeriod * 800);
                    continue;
                }
                pthread_mutex_unlock(&queue->lock);
            }

            if (write(sockFd, packet, packetSize) == -1)
            {
                close(sockFd);
                fprintf(stderr, "CPU sender: Server has no reponse. Stop sending CPU information.\n");
                break;
            }
            // TODO: Logging
            free(packet);
            packet = NULL;
        }
    }
}
