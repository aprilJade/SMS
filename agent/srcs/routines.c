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
    ulong collectPeriodUs = pParam->collectPeriod * 1000;

    sprintf(logmsgBuf, "Start CPU information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(logger, LOG_INFO, logmsgBuf);

    SCData* collectedData;
    
    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, pParam->collectPeriod, pParam->agentId);
        
        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: CPU", elapseTime);
        Log(logger, LOG_DEBUG, logmsgBuf);

        usleep(collectPeriodUs - elapseTime);
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
    SCData* collectedData;
    ulong collectPeriodUs = pParam->collectPeriod * 1000;

    sprintf(logmsgBuf, "Start memory information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(logger, LOG_INFO, logmsgBuf);

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectMemInfo(buf, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: Memory", elapseTime);
        Log(logger, LOG_DEBUG, logmsgBuf);

        usleep(collectPeriodUs - elapseTime);
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
    ulong collectPeriodUs = pParam->collectPeriod * 1000;

    sprintf(logmsgBuf, "Start network information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(logger, LOG_INFO, logmsgBuf);

    SCData* collectedData;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectNetInfo(buf, nicCount, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: Network", elapseTime);
        Log(logger, LOG_DEBUG, logmsgBuf);
        usleep(collectPeriodUs - elapseTime);
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
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    
    sprintf(logmsgBuf, "Start process information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(logger, LOG_INFO, logmsgBuf);

    // TODO: change dynamic dataBuf size
    uchar dataBuf[1024 * 1024] = { 0, };
    SCData* collectedData;
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        memset(dataBuf, 0, 1024 * 1024);
        collectedData = CollectProcInfo(buf, dataBuf, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        uchar* pData = collectedData->data;
        SHeader* hHeader = (SHeader*)pData;

        sprintf(logmsgBuf, "Collected in %ldus: %d Process", elapseTime, hHeader->bodyCount);
        Log(logger, LOG_DEBUG, logmsgBuf);

        usleep(collectPeriodUs - elapseTime);
    }
}

int GetDiskDeviceCount(char* buf)
{
    int fd = open("/proc/diskstats", O_RDONLY);
    if (fd == -1)
        return -1;
    int readSize = read(fd, buf, BUFFER_SIZE);
    if (readSize == -1)
        return -1;
    close(fd);
	int result = 0;
	while (*buf)
	{
		while (*buf == ' ')
			buf++;
		while (*buf != ' ')
			buf++;
		while (*buf == ' ')
			buf++;
		while (*buf != ' ')
			buf++;
		
		buf++;
		if (strncmp(buf, "loop", 4) == 0)
		{
			while(*buf++ != '\n');
			continue;
		}
		result++;
		while(*buf++ != '\n');
	}
	return result;
}

void* DiskInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    char logmsgBuf[128];
    int diskDevCnt = GetDiskDeviceCount(buf);
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    
    sprintf(logmsgBuf, "Start disk information collection routine in %d ms cycle",
        pParam->collectPeriod);
    Log(logger, LOG_INFO, logmsgBuf);

    SCData* collectedData;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectDiskInfo(buf, diskDevCnt, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: Disk", elapseTime);
        Log(logger, LOG_DEBUG, logmsgBuf);
        usleep(collectPeriodUs - elapseTime);
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

    Log(logger, LOG_INFO, "Start sender routine");

    int i = 0;
    while(1)
    {
        while (1)
        {
            sprintf(logmsgBuf, "Try to connect: %s:%d", pParam->host, pParam->port);
            Log(logger, LOG_DEBUG, logmsgBuf);
            if ((sockFd = ConnectToServer(pParam->host, pParam->port)) != -1)
                break;
            connFailCount++;
            sprintf(logmsgBuf, "Failed to connect %s:%d (%d)", pParam->host, pParam->port, connFailCount);
            Log(logger, LOG_ERROR, logmsgBuf);
            sleep(RECONNECT_PERIOD);
        }

        sprintf(logmsgBuf, "Connected to %s:%d", pParam->host, pParam->port);
        Log(logger, LOG_INFO, logmsgBuf);
        connFailCount = 0;

        while (1)
        {
            if (colletecData == NULL)
            {
                if (IsEmpty(queue))
                {
                    // TODO: Remove busy wait or change to the optimized set sleep time 
                    usleep(500);
                    continue;
                }
                
                pthread_mutex_lock(&queue->lock);
                colletecData = Pop(queue);
                pthread_mutex_unlock(&queue->lock);
            }
            
            if ((sendBytes = write(sockFd, colletecData->data, colletecData->dataSize)) == -1)
            {
                close(sockFd);
                sprintf(logmsgBuf, "Disconnected to %s:%d", pParam->host, pParam->port);
                Log(logger, LOG_INFO, logmsgBuf);
                break;
            }
            
            sprintf(logmsgBuf, "Send %d bytes to %s:%d ", sendBytes, pParam->host, pParam->port);
            Log(logger, LOG_DEBUG, logmsgBuf);
            free(colletecData->data);
            free(colletecData);
            colletecData = NULL;
        }
    }
}