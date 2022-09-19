#include "workerRoutine.h"
#include "workerUtils.h"
#include "logger.h"
#include "queue.h"
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
extern pthread_mutex_t g_workerLock;
extern pthread_cond_t g_workerCond;

const int dbConnMaxCnt = 600;
const int dbConnInterval = 1;

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

void RecoverDBConnection(SWorkTools* workTools)
{
    char logMsg[128];
    while (TryConectPg(workTools->dbWrapper, dbConnMaxCnt, dbConnInterval) == false)
    {
        sprintf(logMsg, "%d DB connection is failed %d times in %d second interval",
            workTools->workerId, dbConnMaxCnt, dbConnInterval);
        Log(g_logger, LOG_FATAL, logMsg);
    }
}

void* WorkerRoutine(void* param)
{
    SWorkerParam* pParam = (SWorkerParam*)param;
    char logMsg[128];
    char sqlBuffer[1024];

    sprintf(logMsg, "%d worker-created", pParam->workerId);
    Log(g_logger, LOG_INFO, logMsg);

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
    
    if (workTools.dbWrapper->connected == false)
        RecoverDBConnection(&workTools);
    
    sprintf(logMsg, "%d strat transaction", workTools.workerId);
    Log(g_logger, LOG_DEBUG, logMsg);
    Query(workTools.dbWrapper, "BEGIN");


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
                pthread_mutex_lock(&g_workerLock);
                pthread_cond_wait(&g_workerCond, &g_workerLock);
                pthread_mutex_unlock(&g_workerLock);
                if (g_turnOff)
                    break;
                sprintf(logMsg, "%d Wakeup: New client connected", pParam->workerId);
                Log(g_logger, LOG_INFO, logMsg);
            }
            continue;
        }
        
        if (workTools.queriedSqlCnt >= QUERY_COUNT_THRESHOLD)
        {
            sprintf(logMsg, "%d end transaction", workTools.workerId);
            Log(g_logger, LOG_DEBUG, logMsg);
            Query(workTools.dbWrapper, "END");

            workTools.queriedSqlCnt = 0;

            sprintf(logMsg, "%d strat transaction", workTools.workerId);
            Log(g_logger, LOG_DEBUG, logMsg);
            Query(workTools.dbWrapper, "BEGIN");
        }

        pthread_mutex_lock(&g_queue->lock);
        if (IsEmpty(g_queue))
        {
            pthread_mutex_unlock(&g_queue->lock);
            continue;
        }
        data = Pop(g_queue);
//        printf ("%.2f%%\n", g_queue->cnt / 128.0 * 100.0);
        pthread_mutex_unlock(&g_queue->lock);

        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        hHeader = (SHeader*)data;
        if (InsertFunc[hHeader->signature & EXTRACT_SIGNATURE](sqlBuffer, data, &workTools) == -1)
        {
            workTools.dbWrapper->connected = false;
            sprintf(logMsg, "%d DB connection bad. Try to recover DB connection", workTools.workerId);
            Log(g_logger, LOG_ERROR, logMsg);

            RecoverDBConnection(&workTools);
            
            sprintf(logMsg, "%d DB connection is recovered", workTools.workerId);
            Log(g_logger, LOG_INFO, logMsg);
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