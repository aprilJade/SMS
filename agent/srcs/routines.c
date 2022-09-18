#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include "logger.h"
#include "globalResource.h"

#define AVG_TARGET_TIME_AS_MS 3600000.0 // 1 hour => 60 * 60 * 1000ms
extern SGlobResource globResource;

void* CpuInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
	float toMs = 1000.0 / sysconf(_SC_CLK_TCK);
    ushort cpuCnt = sysconf(_SC_NPROCESSORS_ONLN);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    char logmsgBuf[128];
    ulong* collectPeriod = &globResource.collectPeriods[CPU_COLLECTOR_ID];
    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)*collectPeriod);
    SCData* avgData;
    SCData* collectedData;
    SBodyc* hBody;

    collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, *collectPeriod, globResource.agentID);
    avgData = CalcCpuUtilizationAvg(collectedData->data, cpuCnt, maxCount, toMs, *collectPeriod);
    DestorySCData(&collectedData);
    DestorySCData(&avgData);
    usleep(*collectPeriod * 1000);

    sprintf(logmsgBuf, "Ready CPU information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    while (globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        
        if ((collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, *collectPeriod, globResource.agentID)) == NULL)
        {
            sprintf(logmsgBuf, "CPU: failed to collect");
            Log(globResource.logger, LOG_ERROR, logmsgBuf);
            gettimeofday(&timeVal, NULL);
            postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
            elapseTime = postTime - prevTime;
            usleep(*collectPeriod * 1000 - elapseTime);
            continue;
        }

        avgData = CalcCpuUtilizationAvg(collectedData->data, cpuCnt, maxCount, toMs, *collectPeriod);
        
        if (globResource.bIsConnected)
        {
            pthread_mutex_lock(&globResource.queue->lock);
            Push(collectedData, globResource.queue);
            Push(avgData, globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }
        else
        {
            DestorySCData(&collectedData);
            DestorySCData(&avgData);
        }
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        sprintf(logmsgBuf, "CPU: Collected in %ldus", elapseTime);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);
        
        usleep(*collectPeriod * 1000 - elapseTime);
    }

    Log(globResource.logger, LOG_INFO, "CPU: terminate collector");
}

void* MemInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    char logmsgBuf[128];
    SCData* collectedData;
    ulong* collectPeriod = &globResource.collectPeriods[MEM_COLLECTOR_ID];
    SCData* avgData;
    
    sprintf(logmsgBuf, "Ready memory information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)*collectPeriod);
    
    
    collectedData = CollectMemInfo(buf, *collectPeriod, globResource.agentID);
    avgData = CalcMemAvg(collectedData->data, maxCount);
    DestorySCData(&collectedData);
    DestorySCData(&avgData);
    usleep(*collectPeriod * 1000);

    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        if ((collectedData = CollectMemInfo(buf, *collectPeriod, globResource.agentID)) == NULL)
        {
            sprintf(logmsgBuf, "memory: failed to collect");
            Log(globResource.logger, LOG_ERROR, logmsgBuf);
            gettimeofday(&timeVal, NULL);
            postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
            elapseTime = postTime - prevTime;
            usleep(*collectPeriod * 1000 - elapseTime);
            continue;
        }
        
        avgData = CalcMemAvg(collectedData->data, maxCount);

        if (globResource.bIsConnected)
        {
            pthread_mutex_lock(&globResource.queue->lock);
            Push(collectedData, globResource.queue);
            Push(avgData, globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }
        else
        {
            DestorySCData(&collectedData);
            DestorySCData(&avgData);
        }

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "memory: collected in %ldus", elapseTime);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);

        usleep(*collectPeriod * 1000 - elapseTime);
    }
    Log(globResource.logger, LOG_INFO, "memory: terminate collector");
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
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    char logmsgBuf[128];
    int nicCount = GetNicCount();
    ulong* collectPeriod = &globResource.collectPeriods[NET_COLLECTOR_ID];
    SCData* collectedData;
    SCData* avgData;
    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)*collectPeriod);
    
    collectedData = CollectNetInfo(buf, nicCount, *collectPeriod, globResource.agentID);
    avgData = CalcNetThroughputAvg(collectedData->data, nicCount, maxCount, *collectPeriod);
    DestorySCData(&collectedData);
    DestorySCData(&avgData);
    usleep(*collectPeriod * 1000);

    sprintf(logmsgBuf, "Ready network information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
   
    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        if ((collectedData = CollectNetInfo(buf, nicCount, *collectPeriod, globResource.agentID)) == NULL)
        {
            sprintf(logmsgBuf, "network: failed to collect");
            Log(globResource.logger, LOG_ERROR, logmsgBuf);
            gettimeofday(&timeVal, NULL);
            postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
            elapseTime = postTime - prevTime;
            usleep(*collectPeriod * 1000 - elapseTime);
            continue;
        }

        avgData = CalcNetThroughputAvg(collectedData->data, nicCount, maxCount, *collectPeriod);

        if (globResource.bIsConnected)
        {
            pthread_mutex_lock(&globResource.queue->lock);
            Push(collectedData, globResource.queue);
            Push(avgData, globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }
        else
        {
            DestorySCData(&collectedData);
            DestorySCData(&avgData);
        }        

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "network: collected in %ldus", elapseTime);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);
        usleep(*collectPeriod * 1000 - elapseTime);
    }
    Log(globResource.logger, LOG_INFO, "network: terminate collector");
}

void* ProcInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime, totalElapsed;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    char logmsgBuf[128];
    ulong* collectPeriod = &globResource.collectPeriods[PROC_COLLECTOR_ID];
    
    sprintf(logmsgBuf, "Ready process information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    uchar dataBuf[1024 * 1024] = { 0, };
    SCData* collectedData;
    
    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        memset(dataBuf, 0, 1024 * 1024);
        if ((collectedData = CollectProcInfo(buf, dataBuf, *collectPeriod, globResource.agentID)) == NULL)
        {
            sprintf(logmsgBuf, "process: failed to collect");
            Log(globResource.logger, LOG_ERROR, logmsgBuf);
            gettimeofday(&timeVal, NULL);
            postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
            elapseTime = postTime - prevTime;
            usleep(*collectPeriod * 1000 - elapseTime);
            continue;
        }

        if (globResource.bIsConnected)
        {
            pthread_mutex_lock(&globResource.queue->lock);
            Push(collectedData, globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }
        else
        {
            DestorySCData(&collectedData);
        }
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "process: collected in %ldus", elapseTime);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);

        usleep(*collectPeriod * 1000 - elapseTime);
    }
    Log(globResource.logger, LOG_INFO, "process: terminate collector");
}

void* DiskInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    char logmsgBuf[128];
    ulong* collectPeriod = &globResource.collectPeriods[DISK_COLLECTOR_ID];
    
    sprintf(logmsgBuf, "Ready disk information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    SCData* collectedData;
    
    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        if ((collectedData = CollectDiskInfo(buf, *collectPeriod, globResource.agentID)) == NULL)
        {
            sprintf(logmsgBuf, "disk: failed to collect");
            Log(globResource.logger, LOG_ERROR, logmsgBuf);
            gettimeofday(&timeVal, NULL);
            postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
            elapseTime = postTime - prevTime;
            usleep(*collectPeriod * 1000 - elapseTime);
            continue;
        }

        if (globResource.bIsConnected)
        {
            pthread_mutex_lock(&globResource.queue->lock);
            Push(collectedData, globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }
        else
        {
            DestorySCData(&collectedData);
        }

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "disk: collected in %ldus", elapseTime);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);
        usleep(*collectPeriod * 1000 - elapseTime);
    }
    Log(globResource.logger, LOG_INFO, "disk: terminate collector");
}
