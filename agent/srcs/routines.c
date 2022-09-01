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
#include "routines.h"
#include "collector.h"
#include "avgCalculator.h"

#define RECONNECT_PERIOD 6    // seconds
#define RECONNECT_MAX_TRY 100
#define AVG_TARGET_TIME_AS_MS 3600000.0 // 1 hour => 60 * 60 * 1000ms

extern const Logger* g_logger;
extern Queue* g_queue;
extern const char g_serverIp[16];
extern unsigned short g_serverPort;
static int g_servSockFd;

void* CpuInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
	float toMs = 1000.0 / sysconf(_SC_CLK_TCK);
    ushort cpuCnt = sysconf(_SC_NPROCESSORS_ONLN);
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    char logmsgBuf[128];
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);

    sprintf(logmsgBuf, "Start CPU information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);

    SCData* avgData;
    SCData* collectedData;

    SBodyc* hBody;
    while (1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        
        collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&g_queue->lock);
        Push(collectedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        //avgData = CalcCpuUtilizationAvg(collectedData->data, cpuCnt, maxCount, toMs, pParam->collectPeriod);
//
        //pthread_mutex_lock(&g_queue->lock);
        //Push(avgData, g_queue);
        //pthread_mutex_unlock(&g_queue->lock);
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        sprintf(logmsgBuf, "Collected in %ldus: CPU", elapseTime);
        Log(g_logger, LOG_DEBUG, logmsgBuf);

        usleep(collectPeriodUs - elapseTime);
    }
}

void* MemInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    char logmsgBuf[128];
    SCData* collectedData;
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    SCData* avgData;

    sprintf(logmsgBuf, "Start memory information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);

    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);
    
    SBodym* hBody;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectMemInfo(buf, pParam->collectPeriod, pParam->agentId);
        
        pthread_mutex_lock(&g_queue->lock);
        Push(collectedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        avgData = CalcMemAvg(collectedData->data, maxCount);
        
        pthread_mutex_lock(&g_queue->lock);
        Push(avgData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: Memory", elapseTime);
        Log(g_logger, LOG_DEBUG, logmsgBuf);

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
    char logmsgBuf[128];
    int nicCount = GetNicCount();
    ulong collectPeriodUs = pParam->collectPeriod * 1000;

    sprintf(logmsgBuf, "Start network information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);

    SCData* collectedData;
    SCData* avgData;
    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);
   
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectNetInfo(buf, nicCount, pParam->collectPeriod, pParam->agentId);
        pthread_mutex_lock(&g_queue->lock);
        Push(collectedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);
        
        avgData = CalcNetThroughputAvg(collectedData->data, nicCount, maxCount, pParam->collectPeriod);

        pthread_mutex_lock(&g_queue->lock);
        Push(avgData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: Network", elapseTime);
        Log(g_logger, LOG_DEBUG, logmsgBuf);
        usleep(collectPeriodUs - elapseTime);
    }
}

void* ProcInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime, totalElapsed;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    char logmsgBuf[128];
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    
    sprintf(logmsgBuf, "Start process information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);

    // TODO: change dynamic dataBuf size
    uchar dataBuf[1024 * 1024] = { 0, };
    SCData* collectedData;
    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        memset(dataBuf, 0, 1024 * 1024);
        collectedData = CollectProcInfo(buf, dataBuf, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&g_queue->lock);
        Push(collectedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        uchar* pData = collectedData->data;
        SHeader* hHeader = (SHeader*)pData;

        sprintf(logmsgBuf, "Collected in %ldus: %d Process", elapseTime, hHeader->bodyCount);
        Log(g_logger, LOG_DEBUG, logmsgBuf);

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
    char logmsgBuf[128];
    int diskDevCnt = GetDiskDeviceCount(buf);
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    
    sprintf(logmsgBuf, "Start disk information collection routine in %d ms cycle",
        pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);

    SCData* collectedData;

    while(1)
    {
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        collectedData = CollectDiskInfo(buf, diskDevCnt, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&g_queue->lock);
        Push(collectedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;

        sprintf(logmsgBuf, "Collected in %ldus: Disk", elapseTime);
        Log(g_logger, LOG_DEBUG, logmsgBuf);
        usleep(collectPeriodUs - elapseTime);
    }
}

void RecoverTcpConnection(int signo)
{
    assert(signo == SIGPIPE);
    int connFailCount = 0;
    char logmsgBuf[128];
    sprintf(logmsgBuf, "Disconnected to server: %s:%d", g_serverIp, g_serverPort);
    Log(g_logger, LOG_FATAL, logmsgBuf);
    while (1)
    {
        sprintf(logmsgBuf, "Try to reconnect: %s:%d", g_serverIp, g_serverPort);
        Log(g_logger, LOG_DEBUG, logmsgBuf);
        close(g_servSockFd);
        if ((g_servSockFd = ConnectToServer(g_serverIp, g_serverPort)) != -1)
            break;
        connFailCount++;
        sprintf(logmsgBuf, "Failed to connect %s:%d (%d)", g_serverIp, g_serverPort, connFailCount);
        Log(g_logger, LOG_ERROR, logmsgBuf);
        sleep(RECONNECT_PERIOD);
    }

    sprintf(logmsgBuf, "Connected to %s:%d", g_serverIp, g_serverPort);
    Log(g_logger, LOG_INFO, logmsgBuf);
}

void* SendRoutine(void* param)
{
    SCData* colletecData = NULL;
    int sendBytes;
    char logmsgBuf[128];
    int connFailCount = 0;

	signal(SIGPIPE, RecoverTcpConnection);
    Log(g_logger, LOG_INFO, "Start sender routine");
    
    while (1)
    {
        Log(g_logger, LOG_DEBUG, logmsgBuf);
        if ((g_servSockFd = ConnectToServer(g_serverIp, g_serverPort)) != -1)
            break;
        sprintf(logmsgBuf, "Failed to connect %s:%d (%d)", g_serverIp, g_serverPort, connFailCount);
        connFailCount++;
        Log(g_logger, LOG_ERROR, logmsgBuf);
        sprintf(logmsgBuf, "Try to reconnect: %s:%d", g_serverIp, g_serverPort);
        sleep(RECONNECT_PERIOD);
    }
    sprintf(logmsgBuf, "Connected to %s:%d", g_serverIp, g_serverPort);
    Log(g_logger, LOG_INFO, logmsgBuf);

    int i = 0;
    while(1)
    {
        if (colletecData == NULL)
        {
            if (IsEmpty(g_queue))
            {
                // TODO: Remove busy wait or change to the optimized set sleep time 
                usleep(500);
                continue;
            }
            
            pthread_mutex_lock(&g_queue->lock);
            colletecData = Pop(g_queue);
            pthread_mutex_unlock(&g_queue->lock);
        }
        
        if ((sendBytes = send(g_servSockFd, colletecData->data, colletecData->dataSize, 0)) == -1)
        {
            usleep(1000);
            continue;
        }
        
        sprintf(logmsgBuf, "Send %d bytes to %s:%d ", sendBytes, g_serverIp, g_serverPort);
        Log(g_logger, LOG_DEBUG, logmsgBuf);
        free(colletecData->data);
        free(colletecData);
        colletecData = NULL;
    }
}