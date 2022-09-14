#include "workerRoutine.h"
#include "workerUtils.h"
#include "logger.h"
#include "Queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>

extern bool g_turnOff;
extern const Logger* g_logger;
extern Queue* g_queue;
extern unsigned int g_clientCnt;
extern pthread_mutex_t g_clientCntLock;
extern pthread_cond_t g_clientCntCond;

static const int (*InsertFunc[])(char*, void*, SWorkTools*) = {
    ['c'] = InsertCpuInfo,
    ['C'] = InsertCpuAvgInfo,
    ['m'] = InsertMemInfo,
    ['M'] = InsertMemAvgInfo,
    ['n'] = InsertNetInfo,
    ['N'] = InsertNetAvgInfo,
    ['p'] = InsertProcInfo,
    ['d'] = InsertDiskInfo
};

void* WorkerRoutine(void* param)
{
    SWorkerParam* pParam = (SWorkerParam*)param;
    char logMsg[128];
    char sqlBuffer[1024];

    SHeader* hHeader;
    void* data;
    SWorkTools workTools;
    struct timeval timeVal;
    ulong prevTime, postTime, elapseTime, totalElapsed;
    int pktId;
    workTools.workerId = pParam->workerId;
    workTools.dbWrapper = NewPgWrapper("dbname = postgres port = 5442");
    workTools.threshold = pParam->threshold;
    workTools.queriedSqlCnt = 0;

    while (workTools.dbWrapper->connected == false)
    {
        // TODO: recover connection...
        // Not impelemeted yet
        
    }
    Query(workTools.dbWrapper, "BEGIN");

    sprintf(logMsg, "%d worker-created", pParam->workerId);
    Log(g_logger, LOG_INFO, logMsg);

    while (1)
    {
        if (g_turnOff)
            break;
        

        if (IsEmpty(g_queue))
        {
            usleep(500);
            if (g_clientCnt == 0)
            {
                sprintf(logMsg, "%d Go to sleep: No clients connected, No work remained", pParam->workerId);
                Log(g_logger, LOG_INFO, logMsg);
                pthread_mutex_lock(&g_clientCntLock);
                pthread_cond_wait(&g_clientCntCond, &g_clientCntLock);
                pthread_mutex_unlock(&g_clientCntLock);
                if (g_turnOff)
                    break;
                sprintf(logMsg, "%d Wakeup: New client connected", pParam->workerId);
                Log(g_logger, LOG_INFO, logMsg);
            }
            continue;
        }
        
        if (workTools.queriedSqlCnt >= 128)
        {
            Query(workTools.dbWrapper, "END");
            workTools.queriedSqlCnt = 0;
            Query(workTools.dbWrapper, "BEGIN");
        }

        pthread_mutex_lock(&g_queue->lock);
        if (IsEmpty(g_queue))
        {
            pthread_mutex_unlock(&g_queue->lock);
            continue;
        }
        data = Pop(g_queue);
        pthread_mutex_unlock(&g_queue->lock);

        sprintf(logMsg, "%d work-start", pParam->workerId);
        Log(g_logger, LOG_DEBUG, logMsg);
        
        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        hHeader = (SHeader*)data;
        if (InsertFunc[hHeader->signature & EXTRACT_SIGNATURE](sqlBuffer, data, &workTools) == -1)
        {
            workTools.dbWrapper->connected = false;
            Log(g_logger, LOG_ERROR, "%d DB connection bad");
            continue;
        }

        sprintf(logMsg, "%d query count: %d", pParam->workerId, workTools.queriedSqlCnt);
        Log(g_logger, LOG_DEBUG, logMsg);

        free(data);

        gettimeofday(&timeVal, NULL);
        elapseTime = (timeVal.tv_sec * 1000000 + timeVal.tv_usec) - prevTime;
        sprintf(logMsg, "%d work-done in %ld us", pParam->workerId, elapseTime);
        Log(g_logger, LOG_DEBUG, logMsg);
    }

    sprintf(logMsg, "%d worker-destroied", pParam->workerId);
    Log(g_logger, LOG_INFO, logMsg);
}