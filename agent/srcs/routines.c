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
    char logmsgBuf[128];

    void* data;

    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

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
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        sprintf(logmsgBuf, "INFO: Collected in %ldus: CPU", elapseTime);
        Log(logger, logmsgBuf);

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
    char logmsgBuf[128]; 
    void* data;

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

        sprintf(logmsgBuf, "INFO: Collected in %ldus: Memory", elapseTime);
        Log(logger, logmsgBuf);

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
    char logmsgBuf[128];
    int nicCount = GetNicCount();

    void* data;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        data = CollectNetInfo(buf, nicCount, pParam->collectPeriod);

#if PRINT_NET
        SCData* pd = (SCData*)data;
        SHeader* hHeader = (SHeader*)(pd->data);
        SBodyn* hBody = (SBodyn*)(pd->data + sizeof(SHeader));

        printf("signature: %x, body count: %d, body size: %d, collect period: %d\n",
            hHeader->signature,
            hHeader->bodyCount,
            hHeader->bodySize,
            hHeader->collectPeriod);
        for (int i = 0; i < hHeader->bodyCount; i++)
            printf("%d: %ld B %ld %ld B %ld %d\n",
                i,
                hBody[i].recvBytes,
                hBody[i].recvPackets,
                hBody[i].sendBytes,
                hBody[i].sendPackets,
                hBody[i].nameLength);
#endif

        pthread_mutex_lock(&queue->lock);
        Push(data, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;


        sprintf(logmsgBuf, "INFO: Collected in %ldus: Network", elapseTime);
        Log(logger, logmsgBuf);
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
    char logmsgBuf[128];
    int collectedCount;


    uchar dataBuf[1024 * 1024] = { 0, };
    SCData** data;
    SCData** hData;
    while(1)
    {
        collectedCount = 0;
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;


        memset(dataBuf, 0, 1024 * 1024);
        data = CollectProcInfo(buf, dataBuf, pParam->collectPeriod);
        hData = data;
        while (*hData)
        {
            pthread_mutex_lock(&queue->lock);
            Push(*hData, queue);
            collectedCount++;
            pthread_mutex_unlock(&queue->lock);
            hData++;
        }
        free(data);
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "INFO: Collected %d process in %ldus", elapseTime);
        Log(logger, logmsgBuf);
        usleep(pParam->collectPeriod * 1000 - elapseTime);

        /*
        SCData* pd = (SCData*)data;
        uchar* ppd = pd->data;
        SHeader* hHeader = (SHeader*)ppd;
        //ppd += sizeof(SHeader);
        SBodyp* hBody;
        printf("%x %d %d %d\n", hHeader->signature, hHeader->bodyCount, hHeader->bodySize,
            hHeader->collectPeriod);
        ppd += sizeof(SHeader);
        for (int i = 0; i < hHeader->bodyCount; i++)
        {
            hBody = (SBodyp*)ppd;
            printf("(%d, \'%s\', \'%c\', %d, %d, %d, \'%s\');\n",
                hBody->pid,
                hBody->procName,
                hBody->state,
                hBody->ppid,
                hBody->utime,
                hBody->stime,
                hBody->userName);

            ppd += sizeof(SBodyp);
            if (hBody->cmdlineLen > 0)
            {
                char buf[2048];
                strcpy(buf, ppd);
                printf("%s\n", buf);
                ppd += hBody->cmdlineLen + 1;
            }
        }
        */
    }
}

void* SendRoutine(void* param)
{
    SSenderParam* pParam = (SSenderParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;

    SCData* colletecData = NULL;
    int sockFd, connFailCount = 0;
    int sendBytes;
    char logmsgBuf[128];

    Log(logger, "INFO: Run sender");

    int i = 0;
    while(1)
    {
        while (1)
        {
            sprintf(logmsgBuf, "INFO: Try to connect: %s:%d", pParam->host, pParam->port);
            Log(logger, logmsgBuf);
            if ((sockFd = ConnectToServer(pParam->host, pParam->port)) != -1)
                break;
            connFailCount++;
            sprintf(logmsgBuf, "ERR: Failed to connect %s:%d (%d)", pParam->host, pParam->port, connFailCount);
            Log(logger, logmsgBuf);
            sleep(RECONNECT_PERIOD);
        }

        sprintf(logmsgBuf, "INFO: Connected to %s:%d", pParam->host, pParam->port);
        Log(logger, logmsgBuf);
        connFailCount = 0;
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
                sprintf(logmsgBuf, "ERR: FATAL: Disconnected to %s:%d", pParam->host, pParam->port);
                Log(logger, logmsgBuf);
                break;
            }
            
            sprintf(logmsgBuf, "INFO: Send %d bytes to %s:%d ", sendBytes, pParam->host, pParam->port);
            Log(logger, logmsgBuf);
            free(colletecData->data);
            free(colletecData);
            colletecData = NULL;
        }
    }
}