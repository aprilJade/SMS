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

    LOG_INFO(globResource.logger, "Ready CPU information collection routine in %lu ms cycle", *collectPeriod);
    
    while (globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        
        if ((collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, *collectPeriod, globResource.agentID)) == NULL)
        {
            LOG_ERROR(globResource.logger, "CPU: failed to collect");
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
        LOG_DEBUG(globResource.logger, "CPU: Collected in %ldus", elapseTime);
        
        usleep(*collectPeriod * 1000 - elapseTime);
    }

    LOG_INFO(globResource.logger, "CPU: terminate collector");
}

void* MemInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SCData* collectedData;
    ulong* collectPeriod = &globResource.collectPeriods[MEM_COLLECTOR_ID];
    SCData* avgData;
    
    LOG_INFO(globResource.logger, "Ready memory information collection routine in %lu ms cycle", *collectPeriod);
    
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
            LOG_ERROR(globResource.logger, "memory: failed to collect");
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

        LOG_DEBUG(globResource.logger, "memory: collected in %ldus", elapseTime);

        usleep(*collectPeriod * 1000 - elapseTime);
    }

    LOG_INFO(globResource.logger, "memory: terminate collector");
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

    LOG_INFO(globResource.logger, "Ready network information collection routine in %lu ms cycle", *collectPeriod);
   
    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        if ((collectedData = CollectNetInfo(buf, nicCount, *collectPeriod, globResource.agentID)) == NULL)
        {
            LOG_ERROR(globResource.logger, "network: failed to collect");
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

        LOG_DEBUG(globResource.logger, "network: collected in %ldus", elapseTime);
        usleep(*collectPeriod * 1000 - elapseTime);
    }
    LOG_INFO(globResource.logger, "network: terminate collector");
}

void* ProcInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime, totalElapsed;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    ulong* collectPeriod = &globResource.collectPeriods[PROC_COLLECTOR_ID];
    
    LOG_INFO(globResource.logger, "Ready process information collection routine in %lu ms cycle", *collectPeriod);
    
    uchar dataBuf[1024 * 1024] = { 0, };
    SCData* collectedData;
    
    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        memset(dataBuf, 0, 1024 * 1024);
        if ((collectedData = CollectProcInfo(buf, dataBuf, *collectPeriod, globResource.agentID)) == NULL)
        {
            LOG_ERROR(globResource.logger, "process: failed to collect");
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

        LOG_DEBUG(globResource.logger, "process: collected in %ldus", elapseTime);

        usleep(*collectPeriod * 1000 - elapseTime);
    }
    LOG_INFO(globResource.logger, "process: terminate collector");
}

void* DiskInfoRoutine(void* param)
{
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    ulong* collectPeriod = &globResource.collectPeriods[DISK_COLLECTOR_ID];
    
    LOG_INFO(globResource.logger, "Ready disk information collection routine in %lu ms cycle", *collectPeriod);
    
    SCData* collectedData;
    
    while(globResource.turnOff == false)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        if ((collectedData = CollectDiskInfo(buf, *collectPeriod, globResource.agentID)) == NULL)
        {
            LOG_ERROR(globResource.logger, "disk: failed to collect");
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

        LOG_DEBUG(globResource.logger, "disk: collected in %ldus", elapseTime);
        usleep(*collectPeriod * 1000 - elapseTime);
    }
    LOG_INFO(globResource.logger, "disk: terminate collector");
}
