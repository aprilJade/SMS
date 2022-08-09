#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include "routines.h"
#include "collector.h"

#define RECONNECT_PERIOD 6    // seconds
#define RECONNECT_MAX_TRY 100

#define NO_SLEEP 0
#define PRINT_CPU 0
#define PRINT_MEM 0
#define PRINT_NET 0
#define PRINT_PROC 0


SRoutineParam* GenRoutineParam(int collectPeriod, int collectorID, Queue* queue)
{
    SRoutineParam* ret = (SRoutineParam*)malloc(sizeof(SRoutineParam));
    if (collectPeriod < MIN_SLEEP_MS)
        ret->collectPeriod = MIN_SLEEP_MS;
    else
        ret->collectPeriod = collectPeriod;
    ret->queue = queue;
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
    void* data;
    LoggerOptValue logOptVal;

    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        //CollectCpuInfo(toMs, buf, packet);
        data = CollectEachCpuInfo(cpuCnt, toMs, buf, pParam->collectPeriod);
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
        Push(data, queue);
        pthread_mutex_unlock(&queue->lock);
        printf("push cpu info.\n");
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
    void* data;

    Log(logger, LOG_MEMORY, THRD_CRT, SYS, NO_OPT, NULL);

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        data = CollectMemInfo(buf, pParam->collectPeriod);

#if PRINT_MEM
        printf("<Memory information>\n");
        printf("Free memory: %ld kB\n", packet.memFree);
        printf("Used memory: %ld kB\n", packet.memUsed);
        printf("Available memory: %ld kB\n", packet.memAvail);
        printf("Free swap: %ld kB\n", packet.swapFree);
        printf("Collecting start time: %ld\n\n", packet.collectTime);
#endif

        pthread_mutex_lock(&queue->lock);
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
    void* data;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        data = CollectNetInfo(buf, nicCount, pParam->collectPeriod);

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
    void* data;
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        memset(dataBuf, 0, 1024 * 1024);
        data = CollectProcInfo(buf, dataBuf, pParam->collectPeriod);

        pthread_mutex_lock(&queue->lock);
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
    SSenderParam* pParam = (SSenderParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;

    LoggerOptValue logOptVal;
    logOptVal.connFailCnt = 0;
    logOptVal.queueSize = 0;

    SCData* colletecData = NULL;
    int sockFd, reconnectTryCount = 0;
    int sendBytes;
    
    int i = 0;
    while(1)
    {
        while (1)
        {
            //Log(logger, pParam->collectorID, TRY_CONN, TCP, NO_OPT, NULL);
            if ((sockFd = ConnectToServer(pParam->host, pParam->port)) != -1)
                break;
            logOptVal.connFailCnt++;
            printf("fail to connect.%d\n", i++);
            //Log(logger, pParam->collectorID, FAIL_CONN, TCP, CONN_FAIL_OPT, &logOptVal);
            sleep(RECONNECT_PERIOD);
        }

        //Log(logger, pParam->collectorID, CONN, TCP, NO_OPT, NULL);
        printf("connected.\n");
        while (1)
        {
            if (colletecData == NULL)
            {
                pthread_mutex_lock(&queue->lock);
                while (IsEmpty(queue))
                {
                    pthread_mutex_unlock(&queue->lock);
                    usleep(500);
                    pthread_mutex_lock(&queue->lock);
                }
                colletecData = Pop(queue);
                pthread_mutex_unlock(&queue->lock);
            }
            
            if ((sendBytes = write(sockFd, colletecData->data, colletecData->dataSize)) == -1)
            {
                close(sockFd);
                printf("disconnected.\n");
                //Log(logger, pParam->collectorID, DISCONN, TCP, DISCONN_OPT, &logOptVal);
                break;
            }
            logOptVal.sendBytes = sendBytes;
            printf("send data\n");
            //SHeader* hHeader = (SHeader*)(colletecData->data);
            //SBodyc* hBody = (SBodyc*)(colletecData->data + sizeof(SHeader));
            //for (int i = 0; i < hHeader->bodyCount; i++)
            //{
            //    printf("%d: %012ld %012ld %012ld %012ld\n",
            //        i, hBody->usrCpuRunTime, hBody->sysCpuRunTime,
            //        hBody->idleTime, hBody->waitTime);
            //    hBody++;
            //}
        //
            //Log(logger, pParam->collectorID, SND, TCP, SEND_OPT, &logOptVal);
            free(colletecData->data);
            free(colletecData);
            colletecData = NULL;
        }
    }
}