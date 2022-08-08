#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "routines.h"
#include "collector.h"
#include "tcpCtrl.h"

#define RECONNECT_PERIOD 6    // seconds
#define RECONNECT_MAX_TRY 100

#define NO_SLEEP 0
#define PRINT_CPU 0
#define PRINT_MEM 0
#define PRINT_NET 0
#define PRINT_PROC 0


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

void* CpuInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
	long toMs = 1000 / sysconf(_SC_CLK_TCK);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SCpuInfoPacket* packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    //printf("CPU Collector: Start to collect CPU information every %d ms.\n", pParam->collectPeriod);
    Log(logger, LOG_CPU, THRD_CRT, SYS, NO_OPT, NULL);

    LoggerOptValue logOptVal;
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
        logOptVal.elapseTime = elapseTime;
        Log(logger, LOG_CPU, COLL_COMPLETE, SYS, COLLECT_ELAPSE_OPT, &logOptVal);
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

        CollectMemInfo(buf, packet);

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
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SNetInfoPacket* packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;

    // TODO: Change to Log
    printf("Network Collector: Start to collect network information every %d ms.\n", pParam->collectPeriod);

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        packet = (SNetInfoPacket*)malloc(sizeof(SNetInfoPacket));
        if (packet == NULL)
        {
            // TODO: handling malloc fail
            usleep(5000);
            continue;
        }
        memset(packet, 0, sizeof(SNetInfoPacket));
        packet->collectTime = prevTime / 1000;

        CollectNetInfo(buf, packet);

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

void* ProcInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SProcInfoPacket* packet;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;

    // TODO: Change to Log
    printf("Process Collector: Start to collect process information every %d ms.\n", pParam->collectPeriod);

    DIR* dir;
    struct dirent* entry;
    char path[260];

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
                packet = (SProcInfoPacket*)malloc(sizeof(SProcInfoPacket));
                if (packet == NULL)
                {
                    // TODO: handling malloc fail
                    usleep(5000);
                    continue;
                }
                memset(packet, 0, sizeof(SProcInfoPacket));
                prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

                packet->collectTime = prevTime / 1000;

                if (access(path, F_OK) == 0)
                    CollectProcInfo(path, buf, packet);
                else
                    continue;
                
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

                pthread_mutex_lock(&queue->lock);
                // TODO: Handling Queue is full...more graceful
                while (IsFull(queue))
                {
                    pthread_mutex_unlock(&queue->lock);
                    usleep(5000);
                    pthread_mutex_lock(&queue->lock);
                }
                Push(packet, queue);
                pthread_mutex_unlock(&queue->lock);

                gettimeofday(&timeVal, NULL);
                postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
                elapseTime = postTime - prevTime;
            }
        }
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}

void* SendRoutine(void* param)
{
    SRoutineParam* pParam = (SRoutineParam*)param;
    void (*GenInitPacket)(SInitialPacket*, SRoutineParam*);
    Queue* queue = pParam->queue;
    int packetSize = 0;
    char msgBuf[256];
    char* senderName;
    switch (pParam->collectorID)
    {
        case CPU:
            GenInitPacket = GenerateInitialCpuPacket;
            packetSize = sizeof(SCpuInfoPacket);
            senderName = "CPU Sender";
            break;
        case MEMORY:
            GenInitPacket = GenerateInitialMemPacket;
            packetSize = sizeof(SMemInfoPacket);
            senderName = "Memory Sender";
            break;
        case NETWORK:
            GenInitPacket = GenerateInitialNetPacket;
            packetSize = sizeof(SNetInfoPacket);
            senderName = "Network Sender";
            break;
        case PROCESS:
            GenInitPacket = GenerateInitialProcPacket;
            packetSize = sizeof(SProcInfoPacket);
            senderName = "Process Sender";
            break;
    }

    SInitialPacket initPacket;
    LoggerOptValue logOptVal;
    logOptVal.connFailCnt = 0;
    logOptVal.queueSize = QUEUE_SIZE;
    void* packet = NULL;
    int sockFd, reconnectTryCount = 0;
    Logger* logger = pParam->logger;
    while(1)
    {
        while (1)
        {
            Log(logger, pParam->collectorID, TRY_CONN, TCP, NO_OPT, NULL);
            if ((sockFd = ConnectToServer(HOST, PORT)) != -1)
                break;
            logOptVal.connFailCnt++;
            pthread_mutex_lock(&queue->lock);
            logOptVal.curQueueElemCnt = queue->count;
            pthread_mutex_unlock(&queue->lock);
            Log(logger, pParam->collectorID, FAIL_CONN, TCP, CONN_FAIL_OPT, &logOptVal);
            sleep(RECONNECT_PERIOD);
        }

        Log(logger, pParam->collectorID, CONN, TCP, NO_OPT, NULL);

        GenInitPacket(&initPacket, pParam);
        if (write(sockFd, &initPacket, sizeof(SInitialPacket)) == -1)
        {
            close(sockFd);
            Log(logger, pParam->collectorID, DISCONN, TCP, NO_OPT, NULL);
            continue;
        }

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
                pthread_mutex_lock(&queue->lock);
                logOptVal.curQueueElemCnt = queue->count;
                pthread_mutex_unlock(&queue->lock);
                Log(logger, pParam->collectorID, DISCONN, TCP, DISCONN_OPT, &logOptVal);
                break;
            }
            Log(logger, pParam->collectorID, SND, TCP, NO_OPT, NULL);
            free(packet);
            packet = NULL;
        }
    }
}
