#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdbool.h>
#include "globalResource.h"
#include "avgCalculator.h"

#define RECONNECT_PERIOD 5    // seconds
#define RECONNECT_MAX_TRY 100
#define AVG_TARGET_TIME_AS_MS 3600000.0 // 1 hour => 60 * 60 * 1000ms

extern SGlobResource globResource;

static int g_servSockFd;

void GoToSleep()
{
    pthread_mutex_lock(&globResource.tcpLock);
    pthread_cond_wait(&globResource.tcpCond, &globResource.tcpLock);
    pthread_mutex_unlock(&globResource.tcpLock);
}

void WakeupEveryCollector()
{
    Log(globResource.logger, LOG_DEBUG, "Wakeup every collector");
    pthread_mutex_lock(&globResource.tcpLock);
    pthread_cond_broadcast(&globResource.tcpCond);
    pthread_mutex_unlock(&globResource.tcpLock);
}

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

    sprintf(logmsgBuf, "Ready CPU information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    if (globResource.bIsConnected == false)
    {
        Log(globResource.logger, LOG_INFO, "CPU: wait TCP connection");
        GoToSleep();
        if (globResource.collectorSwitch[CPU_COLLECTOR_ID] == false)
        {
            Log(globResource.logger, LOG_INFO, "CPU: terminate collector");
            return NULL;
        }
    }
    if (globResource.turnOff == true)
    {
        sprintf(logmsgBuf, "CPU: terminate collector");
        Log(globResource.logger, LOG_INFO, logmsgBuf);
    }

    Log(globResource.logger, LOG_INFO, "Run CPU collector: Connected to TCP server");

    while (globResource.turnOff == false && globResource.collectorSwitch[CPU_COLLECTOR_ID] == true)
    {
        if (globResource.bIsConnected == false)
        {
            Log(globResource.logger, LOG_INFO, "CPU: wait TCP connection");
            GoToSleep();
            if (globResource.collectorSwitch[CPU_COLLECTOR_ID] == true)
                Log(globResource.logger, LOG_INFO, "CPU: TCP connection is recovered");
            continue;
        }
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

        pthread_mutex_lock(&globResource.queue->lock);
        Push(collectedData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);

        avgData = CalcCpuUtilizationAvg(collectedData->data, cpuCnt, maxCount, toMs, *collectPeriod);

        pthread_mutex_lock(&globResource.queue->lock);
        Push(avgData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);
        
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
    
    if (globResource.bIsConnected == false)
    {
        Log(globResource.logger, LOG_INFO, "memory: wait TCP connection");
        GoToSleep();
        if (globResource.collectorSwitch[MEM_COLLECTOR_ID] == false)
        {
            Log(globResource.logger, LOG_INFO, "memory: terminate collector");
            return NULL;
        }
    }

    if (globResource.turnOff == true)
    {
        sprintf(logmsgBuf, "memory: terminate collector");
        Log(globResource.logger, LOG_INFO, logmsgBuf);
    }
    Log(globResource.logger, LOG_INFO, "Run memory collector: Connected to TCP server");

    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)*collectPeriod);
    
    SBodym* hBody;

    while(globResource.turnOff == false && globResource.collectorSwitch[MEM_COLLECTOR_ID] == true)
    {
        if (globResource.bIsConnected == false)
        {
            Log(globResource.logger, LOG_INFO, "memory: wait TCP connection");
            GoToSleep();
            if (globResource.collectorSwitch[MEM_COLLECTOR_ID] == true)
                Log(globResource.logger, LOG_INFO, "memory: TCP connection is recovered");
            continue;
        }

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
        
        
        pthread_mutex_lock(&globResource.queue->lock);
        Push(collectedData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);

        avgData = CalcMemAvg(collectedData->data, maxCount);
        
        pthread_mutex_lock(&globResource.queue->lock);
        Push(avgData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);

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

    sprintf(logmsgBuf, "Ready network information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    if (globResource.bIsConnected == false)
    {
        Log(globResource.logger, LOG_INFO, "network: wait TCP connection");
        GoToSleep();
        if (globResource.collectorSwitch[NET_COLLECTOR_ID] == false)
        {
            Log(globResource.logger, LOG_INFO, "network: terminate collector");
            return NULL;
        }
    }

    if (globResource.turnOff == true)
    {
        sprintf(logmsgBuf, "network: terminate collector");
        Log(globResource.logger, LOG_INFO, logmsgBuf);
    }

    Log(globResource.logger, LOG_INFO, "Run network collector: Connected to TCP server");

    while(globResource.turnOff == false && globResource.collectorSwitch[NET_COLLECTOR_ID] == true)
    {
        
        if (globResource.bIsConnected == false)
        {
            Log(globResource.logger, LOG_INFO, "network: wait TCP connection");
            GoToSleep();
            if (globResource.collectorSwitch[NET_COLLECTOR_ID] == true)
                Log(globResource.logger, LOG_INFO, "network: TCP connection is recovered");
            continue;
        }
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

        pthread_mutex_lock(&globResource.queue->lock);
        Push(collectedData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);
        
        avgData = CalcNetThroughputAvg(collectedData->data, nicCount, maxCount, *collectPeriod);

        pthread_mutex_lock(&globResource.queue->lock);
        Push(avgData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);
        
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
    
    if (globResource.bIsConnected == false)
    {
        Log(globResource.logger, LOG_INFO, "process: wait TCP connection");
        GoToSleep();
        if (globResource.collectorSwitch[PROC_COLLECTOR_ID] == false)
        {
            Log(globResource.logger, LOG_INFO, "process: terminate collector");
            return NULL;
        }
    }

    if (globResource.turnOff == true)
    {
        sprintf(logmsgBuf, "process: terminate collector");
        Log(globResource.logger, LOG_INFO, logmsgBuf);
    }
    Log(globResource.logger, LOG_INFO, "Run process collector: Connected to TCP server");

    uchar dataBuf[1024 * 1024] = { 0, };
    SCData* collectedData;

    while(globResource.turnOff == false && globResource.collectorSwitch[PROC_COLLECTOR_ID] == true)
    {
        if (globResource.bIsConnected == false)
        {
            Log(globResource.logger, LOG_INFO, "process: wait TCP connection");
            GoToSleep();
            if (globResource.collectorSwitch[PROC_COLLECTOR_ID] == true)
                Log(globResource.logger, LOG_INFO, "process: TCP connection is recovered");
            continue;
        }
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

        pthread_mutex_lock(&globResource.queue->lock);
        Push(collectedData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        uchar* pData = collectedData->data;
        SHeader* hHeader = (SHeader*)pData;

        sprintf(logmsgBuf, "process: collected in %ldus, count %d ", elapseTime, hHeader->bodyCount);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);

        usleep(*collectPeriod * 1000 - elapseTime);
    }
    Log(globResource.logger, LOG_INFO, "process: terminate collector");
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
    assert(param == NULL);

    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    char logmsgBuf[128];
    int diskDevCnt = GetDiskDeviceCount(buf);
    ulong* collectPeriod = &globResource.collectPeriods[DISK_COLLECTOR_ID];
    
    sprintf(logmsgBuf, "Ready disk information collection routine in %lu ms cycle", *collectPeriod);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    if (globResource.bIsConnected == false)
    {
        Log(globResource.logger, LOG_INFO, "disk: wait TCP connection");
        GoToSleep();
        if (globResource.collectorSwitch[DISK_COLLECTOR_ID] == false)
        {
            Log(globResource.logger, LOG_INFO, "disk: TCP connection is recovered");
            return NULL;
        }
    }

    if (globResource.turnOff == true)
    {
        sprintf(logmsgBuf, "disk: terminate collector");
        Log(globResource.logger, LOG_INFO, logmsgBuf);
    }
    Log(globResource.logger, LOG_INFO, "Run disk collector: Connected to TCP server");

    SCData* collectedData;

    while(globResource.turnOff == false && globResource.collectorSwitch[DISK_COLLECTOR_ID] == true)
    {
        if (globResource.bIsConnected == false)
        {
            Log(globResource.logger, LOG_INFO, "disk: wait TCP connection");
            GoToSleep();
            if (globResource.collectorSwitch[DISK_COLLECTOR_ID] == true)
                Log(globResource.logger, LOG_INFO, "disk: TCP connection is recovered");
            continue;
        }
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        if ((collectedData = CollectDiskInfo(buf, diskDevCnt, *collectPeriod, globResource.agentID)) == NULL)
        {
            sprintf(logmsgBuf, "disk: failed to collect");
            Log(globResource.logger, LOG_ERROR, logmsgBuf);
            gettimeofday(&timeVal, NULL);
            postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
            elapseTime = postTime - prevTime;
            usleep(*collectPeriod * 1000 - elapseTime);
            continue;
        }

        pthread_mutex_lock(&globResource.queue->lock);
        Push(collectedData, globResource.queue);
        pthread_mutex_unlock(&globResource.queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "disk: collected in %ldus", elapseTime);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);
        usleep(*collectPeriod * 1000 - elapseTime);
    }
    Log(globResource.logger, LOG_INFO, "disk: terminate collector");
}


void RecoverTcpConnection(int signo)
{
    assert(signo == SIGPIPE);
    globResource.bIsConnected = false;

    int connFailCount = 0;
    char logmsgBuf[128];
    sprintf(logmsgBuf, "Disconnected to server: %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_FATAL, logmsgBuf);

    sprintf(logmsgBuf, "Try to reconnect: %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_DEBUG, logmsgBuf);
    while (1)
    {
        if (globResource.turnOff)
        {
            WakeupEveryCollector();
            return;
        }
        close(g_servSockFd);
        if ((g_servSockFd = ConnectToServer(globResource.peerIP, globResource.peerPort)) != -1)
            break;
        connFailCount++;
        sprintf(logmsgBuf, "Failed to connect %s:%d (%d)", globResource.peerIP, globResource.peerPort, connFailCount);
        Log(globResource.logger, LOG_ERROR, logmsgBuf);
        sleep(RECONNECT_PERIOD);
    }
    
    sprintf(logmsgBuf, "Re-connected to %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    globResource.bIsConnected = true;
    WakeupEveryCollector();
}

void* SendRoutine(void* param)
{
    SCData* collectedData = NULL;
    int sendBytes;
    char logmsgBuf[128];
    int connFailCount = 0;

	signal(SIGPIPE, RecoverTcpConnection);
    Log(globResource.logger, LOG_INFO, "Start sender routine");

    globResource.bIsConnected = false;

    while (1)
    {
        if (globResource.turnOff)
        {
            WakeupEveryCollector();
            sprintf(logmsgBuf, "Terminate sender");
            Log(globResource.logger, LOG_INFO, logmsgBuf);
            return NULL;
        }
        if ((g_servSockFd = ConnectToServer(globResource.peerIP, globResource.peerPort)) != -1)
            break;
        sprintf(logmsgBuf, "Failed to connect %s:%d (%d)", globResource.peerIP, globResource.peerPort, connFailCount);
        connFailCount++;
        Log(globResource.logger, LOG_ERROR, logmsgBuf);
        sprintf(logmsgBuf, "Try to reconnect: %s:%d", globResource.peerIP, globResource.peerPort);
        sleep(RECONNECT_PERIOD);
    }

    sprintf(logmsgBuf, "Connected to %s:%d", globResource.peerIP, globResource.peerPort);
    Log(globResource.logger, LOG_INFO, logmsgBuf);
    
    globResource.bIsConnected = true;
    WakeupEveryCollector();

    while(globResource.turnOff == false)
    {
        if (collectedData == NULL)
        {
            if (IsEmpty(globResource.queue))
            {
                usleep(500000);
                continue;
            }
            
            pthread_mutex_lock(&globResource.queue->lock);
            collectedData = Pop(globResource.queue);
            pthread_mutex_unlock(&globResource.queue->lock);
        }
        
        if ((sendBytes = send(g_servSockFd, collectedData->data, collectedData->dataSize, 0)) == -1)
            continue;
        
        sprintf(logmsgBuf, "Send %d bytes to %s:%d ", sendBytes, globResource.peerIP, globResource.peerPort);
        Log(globResource.logger, LOG_DEBUG, logmsgBuf);
        DestorySCData(&collectedData);
    }
    
    sprintf(logmsgBuf, "Terminate sender");
    Log(globResource.logger, LOG_INFO, logmsgBuf);
}