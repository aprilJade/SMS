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
#include "routines.h"
#include "collector.h"
#include "avgCalculator.h"

#define RECONNECT_PERIOD 5    // seconds
#define RECONNECT_MAX_TRY 100
#define AVG_TARGET_TIME_AS_MS 3600000.0 // 1 hour => 60 * 60 * 1000ms

extern const Logger* g_logger;
extern Queue* g_queue;
extern const char g_serverIp[16];
extern unsigned short g_serverPort;
extern bool g_turnOff;
static int g_servSockFd;

static bool g_connected = false;
static pthread_mutex_t g_condLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;

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

    sprintf(logmsgBuf, "Ready CPU information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);
    
    if (g_connected == false)
    {
        Log(g_logger, LOG_INFO, "Wait CPU collector: Wait TCP connection");
        pthread_mutex_lock(&g_condLock);
        pthread_cond_wait(&g_cond, &g_condLock);
        pthread_mutex_unlock(&g_condLock);
    }
    if (g_turnOff == true)
    {
        sprintf(logmsgBuf, "Terminate CPU collector");
        Log(g_logger, LOG_INFO, logmsgBuf);
    }
    Log(g_logger, LOG_INFO, "Run CPU collector: Connected to TCP server");

    SCData* avgData;
    SCData* collectedData;
    SBodyc* hBody;

    while (1)
    {
        if (g_turnOff == true)
            break;
        if (g_connected == false)
        {
            Log(g_logger, LOG_ERROR, "Pause CPU collector: TCP connection BAD");
            pthread_mutex_lock(&g_condLock);
            pthread_cond_wait(&g_cond, &g_condLock);
            pthread_mutex_unlock(&g_condLock);
            Log(g_logger, LOG_INFO, "Resume CPU collector: TCP connection recovered");
            continue;
        }
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;
        
        collectedData = CollectEachCpuInfo(cpuCnt, toMs, buf, pParam->collectPeriod, pParam->agentId);

        pthread_mutex_lock(&g_queue->lock);
        Push(collectedData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        avgData = CalcCpuUtilizationAvg(collectedData->data, cpuCnt, maxCount, toMs, pParam->collectPeriod);

        pthread_mutex_lock(&g_queue->lock);
        Push(avgData, g_queue);
        pthread_mutex_unlock(&g_queue->lock);
        
        gettimeofday(&timeVal, NULL);
        postTime = timeVal.tv_sec * 1000000  + timeVal.tv_usec;
        elapseTime = postTime - prevTime;
        sprintf(logmsgBuf, "Collected in %ldus: CPU", elapseTime);
        Log(g_logger, LOG_DEBUG, logmsgBuf);
        
        usleep(collectPeriodUs - elapseTime);
    }
    sprintf(logmsgBuf, "Terminate CPU collector");
    Log(g_logger, LOG_INFO, logmsgBuf);
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

    
    sprintf(logmsgBuf, "Ready memory information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);
    
    if (g_connected == false)
    {
        Log(g_logger, LOG_INFO, "Wait memory collector: Wait TCP connection");
        pthread_mutex_lock(&g_condLock);
        pthread_cond_wait(&g_cond, &g_condLock);
        pthread_mutex_unlock(&g_condLock);
    }
    if (g_turnOff == true)
    {
        sprintf(logmsgBuf, "Terminate memory collector");
        Log(g_logger, LOG_INFO, logmsgBuf);
    }
    Log(g_logger, LOG_INFO, "Run memory collector: Connected to TCP server");

    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);
    
    SBodym* hBody;

    while(1)
    {
        if (g_turnOff == true)
            break;
        if (g_connected == false)
        {
            Log(g_logger, LOG_ERROR, "Pause memory collector: TCP connection BAD");
            pthread_mutex_lock(&g_condLock);
            pthread_cond_wait(&g_cond, &g_condLock);
            pthread_mutex_unlock(&g_condLock);
            Log(g_logger, LOG_INFO, "Resume memory collector: TCP connection recovered");
            continue;
        }

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
    sprintf(logmsgBuf, "Terminate memory collector");
    Log(g_logger, LOG_INFO, logmsgBuf);
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

    sprintf(logmsgBuf, "Ready network information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);
    
    if (g_connected == false)
    {
        Log(g_logger, LOG_INFO, "Wait network collector: Wait TCP connection");
        pthread_mutex_lock(&g_condLock);
        pthread_cond_wait(&g_cond, &g_condLock);
        pthread_mutex_unlock(&g_condLock);
    }
    if (g_turnOff == true)
    {
        sprintf(logmsgBuf, "Terminate network collector");
        Log(g_logger, LOG_INFO, logmsgBuf);
    }
    Log(g_logger, LOG_INFO, "Run network collector: Connected to TCP server");

    SCData* collectedData;
    SCData* avgData;
    int maxCount = (int)(AVG_TARGET_TIME_AS_MS / (float)pParam->collectPeriod);
   
    while(1)
    {
        if (g_turnOff == true)
            break;
        if (g_connected == false)
        {
            Log(g_logger, LOG_ERROR, "Pause network collector: TCP connection BAD");
            pthread_mutex_lock(&g_condLock);
            pthread_cond_wait(&g_cond, &g_condLock);
            pthread_mutex_unlock(&g_condLock);
            Log(g_logger, LOG_INFO, "Resume network collector: TCP connection recovered");
            continue;
        }
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
    sprintf(logmsgBuf, "Terminate network collector");
    Log(g_logger, LOG_INFO, logmsgBuf);
}

void* ProcInfoRoutine(void* param)
{
    ulong prevTime, postTime, elapseTime, totalElapsed;
    char buf[BUFFER_SIZE + 1] = { 0, };
    struct timeval timeVal;
    SRoutineParam* pParam = (SRoutineParam*)param;
    char logmsgBuf[128];
    ulong collectPeriodUs = pParam->collectPeriod * 1000;
    
    sprintf(logmsgBuf, "Ready process information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);
    
    if (g_connected == false)
    {
        Log(g_logger, LOG_INFO, "Wait process collector: Wait TCP connection");
        pthread_mutex_lock(&g_condLock);
        pthread_cond_wait(&g_cond, &g_condLock);
        pthread_mutex_unlock(&g_condLock);
    }
    if (g_turnOff == true)
    {
        sprintf(logmsgBuf, "Terminate process collector");
        Log(g_logger, LOG_INFO, logmsgBuf);
    }
    Log(g_logger, LOG_INFO, "Run process collector: Connected to TCP server");

    // TODO: change dynamic dataBuf size
    uchar dataBuf[1024 * 1024] = { 0, };
    SCData* collectedData;
    while(1)
    {
        if (g_turnOff)
            break;
        if (g_connected == false)
        {
            Log(g_logger, LOG_ERROR, "Pause process collector: TCP connection BAD");
            pthread_mutex_lock(&g_condLock);
            pthread_cond_wait(&g_cond, &g_condLock);
            pthread_mutex_unlock(&g_condLock);
            Log(g_logger, LOG_INFO, "Resume process collector: TCP connection recovered");
            continue;
        }
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
    sprintf(logmsgBuf, "Terminate process collector");
    Log(g_logger, LOG_INFO, logmsgBuf);
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
    
    sprintf(logmsgBuf, "Ready disk information collection routine in %d ms cycle", pParam->collectPeriod);
    Log(g_logger, LOG_INFO, logmsgBuf);
    
    if (g_connected == false)
    {
        Log(g_logger, LOG_INFO, "Wait disk collector: Wait TCP connection");
        pthread_mutex_lock(&g_condLock);
        pthread_cond_wait(&g_cond, &g_condLock);
        pthread_mutex_unlock(&g_condLock);
    }
    if (g_turnOff == true)
    {
        sprintf(logmsgBuf, "Terminate disk collector");
        Log(g_logger, LOG_INFO, logmsgBuf);
    }
    Log(g_logger, LOG_INFO, "Run disk collector: Connected to TCP server");

    SCData* collectedData;

    while(1)
    {
        if (g_turnOff == true)
            break;
        if (g_connected == false)
        {
            Log(g_logger, LOG_ERROR, "Pause disk collector: TCP connection BAD");
            pthread_mutex_lock(&g_condLock);
            pthread_cond_wait(&g_cond, &g_condLock);
            pthread_mutex_unlock(&g_condLock);
            Log(g_logger, LOG_INFO, "Resume disk collector: TCP connection recovered");
            continue;
        }
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
    sprintf(logmsgBuf, "Terminate DISK collector");
    Log(g_logger, LOG_INFO, logmsgBuf);
}

void RecoverTcpConnection(int signo)
{
    assert(signo == SIGPIPE);
    g_connected = false;

    int connFailCount = 0;
    char logmsgBuf[128];
    sprintf(logmsgBuf, "Disconnected to server: %s:%d", g_serverIp, g_serverPort);
    Log(g_logger, LOG_FATAL, logmsgBuf);
    while (1)
    {
        if (g_turnOff)
        {
            pthread_mutex_lock(&g_condLock);
            pthread_cond_broadcast(&g_cond);
            pthread_mutex_unlock(&g_condLock);
            sprintf(logmsgBuf, "Terminate Sender");
            Log(g_logger, LOG_INFO, logmsgBuf);
            return;
        }
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
    g_connected = true;
    pthread_mutex_lock(&g_condLock);
    pthread_cond_broadcast(&g_cond);
    pthread_mutex_unlock(&g_condLock);
}

void* SendRoutine(void* param)
{
    SCData* colletecData = NULL;
    int sendBytes;
    char logmsgBuf[128];
    int connFailCount = 0;

	signal(SIGPIPE, RecoverTcpConnection);
    Log(g_logger, LOG_INFO, "Start sender routine");

    g_connected = false;
    while (1)
    {
        if (g_turnOff)
        {
            pthread_mutex_lock(&g_condLock);
            pthread_cond_broadcast(&g_cond);
            pthread_mutex_unlock(&g_condLock);
            sprintf(logmsgBuf, "Terminate Sender");
            Log(g_logger, LOG_INFO, logmsgBuf);
            return NULL;
        }
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
    g_connected = true;
    pthread_mutex_lock(&g_condLock);
    pthread_cond_broadcast(&g_cond);
    pthread_mutex_unlock(&g_condLock);

    int i = 0;
    while(1)
    {
        if (g_turnOff)
            break;
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
    
    sprintf(logmsgBuf, "Terminate Sender");
    Log(g_logger, LOG_INFO, logmsgBuf);
}