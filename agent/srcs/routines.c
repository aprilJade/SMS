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
#define AVG_TARGET_TIME_AS_MS 3600000.0 // 1 hour => 60 * 60 * 1000ms


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

ulong CalcTotalCpuIdleTime(SCData* collectedData)
{
    SHeader* hHeader = (SHeader*)collectedData->data;
    SBodyc* hBody = (SBodyc*)(collectedData->data + sizeof(SHeader));
    ulong totalIdle = 0;
    for (int i = 0; i < hHeader->bodyCount; i++)
        totalIdle += hBody[i].idleTime;
    return totalIdle;
}

float round(float x)
{
    x += 0.005;
    int tmp = (int)(x * 100);
    x = tmp / 100.0;
    return x;
}

SCData* MakeCpuAvgPacket(uchar* collectedData, int cpuCnt, float* utilization, float* avg)
{
    SCData* avgData = (SCData*)malloc(sizeof(SCData));
    avgData->dataSize = sizeof(SHeader) + sizeof(SBodyAvgC) * cpuCnt;
    avgData->data = (uchar*)malloc(avgData->dataSize);

    memcpy(collectedData, avgData->data, sizeof(SHeader));
    SHeader* hHeader = (SHeader*)avgData->data;
    SBodyAvgC* hBody = (SBodyAvgC*)(avgData->data + sizeof(SHeader));
    hHeader->signature = SIGNATURE_AVG_CPU;
    for (int i = 0; i < cpuCnt; i++)
    {
        hBody[i].avgUtilization = avg[i];
        hBody[i].cpuUtilization = utilization[i];
    }
    return avgData;
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

    SCData* avgData;
    SCData* collectedData;
    ulong* curIdle = (ulong*)malloc(sizeof(ulong) * cpuCnt);
    ulong* prevIdle = (ulong*)malloc(sizeof(ulong) * cpuCnt);
    float* deltaIdle = (ulong*)malloc(sizeof(float) * cpuCnt);

    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);
    float** cpuUtilizations = (float**)malloc(sizeof(float*) * maxCount);
    for (int i = 0; i < maxCount; i++)
    {
        cpuUtilizations[i] = (float*)malloc(sizeof(float) * cpuCnt);
        memset(cpuUtilizations[i], 0, sizeof(float) * cpuCnt);
    }
    int curCount = 0;
    int idx = 0;
    float* avg = (float*)malloc(sizeof(float) * cpuCnt);
    memset(avg, 0, sizeof(float) * cpuCnt);

    SBodyc* hBody;
    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        
        collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, pParam->collectPeriod, pParam->agentId);

        // Push to collected data
        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        // calculate cpu utilization and that average value
        hBody = (SBodyc*)(collectedData->data + sizeof(SHeader));

        for (int i = 0; i < cpuCnt; i++)
        {
            curIdle[i] = hBody[i].idleTime;
            deltaIdle[i] = (float)(curIdle[i] - prevIdle[i]) * (float)toMs;
            prevIdle[i] = curIdle[i];
            cpuUtilizations[idx][i] = round(100.0 - ((deltaIdle[i]) / (float)pParam->collectPeriod * 100.0));
            if (cpuUtilizations[idx][i] < 0)
                cpuUtilizations[idx][i] = 0;
            avg[i] = 0;
            for (int j = 0; j < curCount; j++)
                avg[i] += cpuUtilizations[j][i];
            avg[i] = round(avg[i] / (float)curCount);
            // printf("%d: Cpu utilization average: %f%%\n", i, avg[i]);
            // printf("%d: CPU Usage: %f%%\n", i, cpuUtilizations[idx][i]);
        }
        if (curCount != 0)
        {
            // avgData = MakeCpuAvgPacket(collectedData->data, cpuCnt, cpuUtilizations[idx], avg);
            // pthread_mutex_lock(&queue->lock);
            // Push(avgData, queue);
            // pthread_mutex_unlock(&queue->lock);
            idx++;
            idx %= maxCount;
        }
        if (curCount < maxCount)
            curCount++;

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        sprintf(logmsgBuf, "Collected in %ldus: CPU", elapseTime);
        Log(logger, LOG_DEBUG, logmsgBuf);

        usleep(collectPeriodUs - elapseTime);
    }
}

SCData* MakeMemAvgPacket(uchar* collectedData, float memUsage, float memAvg, float swapUsage, float swapAvg)
{
    SCData* avgData = (SCData*)malloc(sizeof(SCData));
    avgData->dataSize = sizeof(SHeader) + sizeof(SBodyAvgM);
    avgData->data = (uchar*)malloc(avgData->dataSize);
    
    memcpy(avgData->data, collectedData, sizeof(SHeader));
    SHeader* hHeader = (SHeader*)avgData->data;
    hHeader->signature = SIGNATURE_AVG_MEM;

    SBodyAvgM* hBody = (SBodyAvgM*)(avgData->data + sizeof(SHeader));
    hBody->memUsage = memUsage;
    hBody->memUsageAvg = memAvg;
    hBody->swapUsage = swapUsage;
    hBody->swapUsageAvg = swapAvg;

    return avgData;
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
    SCData* avgData;

    sprintf(logmsgBuf, "Start memory information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(logger, LOG_INFO, logmsgBuf);

    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);
    int curCount = 1;
    float* memUsage = (float*)malloc(sizeof(float) * maxCount);
    float* swapUsage = (float*)malloc(sizeof(float) * maxCount);
    float memAvg;
    float swapAvg;

    SBodym* hBody;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectMemInfo(buf, pParam->collectPeriod, pParam->agentId);
        
        pthread_mutex_lock(&queue->lock);
        Push(collectedData, queue);
        pthread_mutex_unlock(&queue->lock);

        hBody = (SBodym*)(collectedData->data + sizeof(SHeader));
        
        memUsage[curCount - 1] = round((float)(hBody->memTotal - hBody->memAvail) / (float)hBody->memTotal * 100);
        swapUsage[curCount - 1] = round((float)(hBody->swapTotal - hBody->swapFree) / (float)hBody->swapTotal * 100);
        memAvg = 0;
        swapAvg = 0;
        for (int i = 0; i < curCount; i++)
        {
            memAvg += memUsage[i];
            swapAvg += swapUsage[i];
        }
        memAvg = round(memAvg / (float)curCount);
        swapAvg = round(swapAvg / (float)curCount);
        
        printf("Memory usage: %.2f%%\n", memUsage[curCount - 1]);
        printf("Memory avg: %.2f%%\n", memAvg);
        printf("Swap usage: %.2f%%\n", swapUsage[curCount - 1]);
        printf("Swap avg: %.2f%%\n", swapAvg);
        if (curCount < maxCount - 1)
            curCount++;
        avgData = MakeMemAvgPacket(collectedData->data, memUsage[curCount], memAvg, swapUsage[curCount], swapAvg);
        
        pthread_mutex_lock(&queue->lock);
        Push(avgData, queue);
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
    
    ulong prevRecvBytes;
    ulong curRecvBytes;
    float recvBytesPerSec;

    ulong prevRecvPackets;
    ulong curRecvPackets;
    float recvPacketsPerSec;

    ulong prevSendBytes;
    ulong curSendBytes;
    float sendBytesPerSec;

    ulong prevSendPackets;
    ulong curSendPackets;
    float sendPacketsPerSec;

    SHeader* hHeader;
    SBodyn* hBody;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectNetInfo(buf, nicCount, pParam->collectPeriod, pParam->agentId);
        
        hHeader = (SHeader*)(collectedData->data);
        hBody = (SBodyn*)(collectedData->data + sizeof(SHeader));
        int lastIdx = hHeader->bodyCount - 1;
        //printf("<=== %s information ===>\n", hBody[lastIdx].name);

        curRecvBytes = hBody[lastIdx].recvBytes;
        recvBytesPerSec = (float)(curRecvBytes - prevRecvBytes) / (float)pParam->collectPeriod * 1000.0;
        prevRecvBytes = curRecvBytes;
        //printf("Received bytes: %f B/s\n", recvBytesPerSec);

        curRecvPackets = hBody[lastIdx].recvPackets;
        recvPacketsPerSec = (float)(curRecvPackets - prevRecvPackets) / (float)pParam->collectPeriod * 1000.0;
        prevRecvPackets = curRecvPackets;
        //printf("Received packets: %f per s\n", recvPacketsPerSec);

        curSendBytes = hBody[lastIdx].sendBytes;
        sendBytesPerSec = (float)(curSendBytes - prevSendBytes) / (float)pParam->collectPeriod * 1000.0;
        prevSendBytes = curSendBytes;
        //printf("Send bytes: %f B/s\n", sendBytesPerSec);

        curSendPackets = hBody[lastIdx].sendPackets;
        sendPacketsPerSec = (float)(curSendPackets - prevSendPackets) / (float)pParam->collectPeriod * 1000.0;
        prevSendPackets =curSendPackets;
        //printf("Send packets: %f per s\n", sendPacketsPerSec);

        
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
		if (strncmp(buf, "loop", 4) == 0 || strncmp(buf, "ram", 3) == 0)
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