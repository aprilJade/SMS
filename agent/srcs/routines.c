#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
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
        ret->collectPeriod = MIN_SLEEP_MS;
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
    ushort cpuCnt = sysconf(_SC_NPROCESSORS_ONLN);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;

    Log(logger, LOG_CPU, THRD_CRT, SYS, NO_OPT, NULL);
    uchar* data;
    LoggerOptValue logOptVal;

    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        //CollectCpuInfo(toMs, buf, packet);
        data = CollectEachCpuInfo(cpuCnt, toMs, buf);
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
        Push(data, queue);
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
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    LoggerOptValue logOptVal;
    uchar* data;

    Log(logger, LOG_MEMORY, THRD_CRT, SYS, NO_OPT, NULL);

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        data = CollectMemInfo(buf);

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
        Push(data, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        logOptVal.elapseTime = elapseTime;
        Log(logger, LOG_MEMORY, COLL_COMPLETE, SYS, COLLECT_ELAPSE_OPT, &logOptVal);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}

int GetNicCount()
{
    char buf[BUFFER_SIZE] = { 0, };
    int fd = open("/proc/net/dev", O_RDONLY);
	if (fd == -1)
	{
		// TODO: handling open error
		perror("agent");
		return -1;
	}
    char* pBuf = buf;
	int readSize = read(fd, buf, BUFFER_SIZE);
	if (readSize == -1)
	{
		// TODO: handling read error
		perror("agent");
		return -1;
	}
	buf[readSize] = 0;
    int cnt = 0;
	while (*pBuf++ != '\n');
	while (*pBuf++ != '\n');
    while (*pBuf)
    {
        cnt++;
	    while (*pBuf++ != '\n');
    }
    return cnt;
}

void* NetInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    LoggerOptValue logOptVal;

    int nicCount = GetNicCount();

    Log(logger, LOG_NETWORK, THRD_CRT, SYS, NO_OPT, NULL);
    uchar* data;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        data = CollectNetInfo(buf, nicCount);

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
        Push(data, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        logOptVal.elapseTime = elapseTime;
        Log(logger, LOG_NETWORK, COLL_COMPLETE, SYS, COLLECT_ELAPSE_OPT, &logOptVal);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}

void* ProcInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime, totalElapsed;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    LoggerOptValue logOptVal;
    int collectedCount;

    Log(logger, LOG_PROCESS, THRD_CRT, SYS, NO_OPT, NULL);

    uchar dataBuf[1024 * 1024] = { 0, };
    uchar* data;
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        memset(dataBuf, 0, 1024 * 1024);
        data = CollectProcInfo(buf, dataBuf);

        pthread_mutex_lock(&queue->lock);
        // TODO: Handling Queue is full...more graceful
        while (IsFull(queue))
            usleep(500);
        Push(data, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        logOptVal.elapseTime = elapseTime;
        Log(logger, LOG_PROCESS, COLL_COMPLETE, SYS, COLLECT_ELAPSE_OPT, &logOptVal);
        usleep(pParam->collectPeriod * 1000 - elapseTime);
    }
}

void* SendRoutine(void* param)
{
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    int packetSize = 0;
    char msgBuf[256];
    SInitialPacket* initialPacket;
    int initialPacketSize;
    switch (pParam->collectorID)
    {
        case CPU:
            initialPacket = (SInitialPacket*)GenerateInitialCpuPacket(pParam);
            packetSize = sizeof(SHeader) + sysconf(_SC_NPROCESSORS_ONLN) * sizeof(SBodyc);
            initialPacketSize = sizeof(SInitialPacket);
            break;
        case MEMORY:
            initialPacket = (SInitialPacket*)GenerateInitialMemPacket(pParam);
            packetSize = sizeof(SHeader) + sizeof(SBodym);
            initialPacketSize = sizeof(SInitialPacket);
            break;
        case NETWORK:
            initialPacket = (SInitialPacket*)GenerateInitialNetPacket(pParam);
            packetSize = sizeof(SHeader) + sizeof(SBodyn) * GetNicCount();
            initialPacketSize = sizeof(SInitialPacket) + sizeof(SInitialPacketBody) * GetNicCount();
            break;
        case PROCESS:
            initialPacket = (SInitialPacket*)GenerateInitialProcPacket(pParam);
            initialPacketSize = sizeof(SInitialPacket);
            break;
    }

    LoggerOptValue logOptVal;
    logOptVal.connFailCnt = 0;
    logOptVal.queueSize = QUEUE_SIZE;
    void* data = NULL;
    int sockFd, reconnectTryCount = 0;
    Logger* logger = pParam->logger;
    int sendBytes;
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

        if (write(sockFd, initialPacket, initialPacketSize) == -1)
        {
            close(sockFd);
            Log(logger, pParam->collectorID, DISCONN, TCP, NO_OPT, NULL);
            continue;
        }
        initialPacket->isReconnected = 1;

        while (1)
        {
            if (data == NULL)
            {
                pthread_mutex_lock(&queue->lock);
                while (IsEmpty(queue))
                {
                    pthread_mutex_unlock(&queue->lock);
                    usleep(pParam->collectPeriod * 800);
                    pthread_mutex_lock(&queue->lock);
                }
                data = Pop(queue);
                pthread_mutex_unlock(&queue->lock);
            }
            
            if (pParam->collectorID == PROCESS)
            {
                SHeader* handle = (SHeader*)data;
                packetSize = handle->bodySize;
            }

            if ((sendBytes = write(sockFd, data, packetSize)) == -1)
            {
                close(sockFd);
                pthread_mutex_lock(&queue->lock);
                logOptVal.curQueueElemCnt = queue->count;
                pthread_mutex_unlock(&queue->lock);
                Log(logger, pParam->collectorID, DISCONN, TCP, DISCONN_OPT, &logOptVal);
                break;
            }
            logOptVal.sendBytes = sendBytes;
            Log(logger, pParam->collectorID, SND, TCP, SEND_OPT, &logOptVal);
            free(data);
            data = NULL;
        }
    }
}