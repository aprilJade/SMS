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
    while (TryConectPg(workTools->dbWrapper, dbConnMaxCnt, dbConnInterval) == false)
    {
        LOG_FATAL(g_logger, "%d DB connection is failed %d times in %d second interval",
            workTools->workerId, dbConnMaxCnt, dbConnInterval);
    }
}

void* WorkerRoutine(void* param)
{
    SWorkerParam* pParam = (SWorkerParam*)param;
    char sqlBuffer[1024];

    LOG_INFO(g_logger, "%d worker-created", pParam->workerId);

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
    
    LOG_DEBUG(g_logger, "%d strat transaction", workTools.workerId);
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
                LOG_INFO(g_logger, "%d Go to sleep: No clients connected, No work remained", pParam->workerId);
                pthread_mutex_lock(&g_workerLock);
                pthread_cond_wait(&g_workerCond, &g_workerLock);
                pthread_mutex_unlock(&g_workerLock);
                if (g_turnOff)
                    break;
                LOG_INFO(g_logger, "%d Wakeup: New client connected", pParam->workerId);
            }
            continue;
        }
        
        if (workTools.queriedSqlCnt >= QUERY_COUNT_THRESHOLD)
        {
            LOG_DEBUG(g_logger, "%d end transaction", workTools.workerId);
            Query(workTools.dbWrapper, "END");

            workTools.queriedSqlCnt = 0;

            LOG_DEBUG(g_logger, "%d strat transaction", workTools.workerId);
            Query(workTools.dbWrapper, "BEGIN");
        }

        pthread_mutex_lock(&g_queue->lock);
        if (IsEmpty(g_queue))
        {
            pthread_mutex_unlock(&g_queue->lock);
            continue;
        }
        data = Pop(g_queue);
        printf("%d\n", g_queue->cnt);
        pthread_mutex_unlock(&g_queue->lock);

        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        hHeader = (SHeader*)data;
        if (InsertFunc[hHeader->signature & EXTRACT_SIGNATURE](sqlBuffer, data, &workTools) == -1)
        {
            workTools.dbWrapper->connected = false;
            LOG_ERROR(g_logger, "%d DB connection bad. Try to recover DB connection", workTools.workerId);

            RecoverDBConnection(&workTools);
            
            LOG_INFO(g_logger, "%d DB connection is recovered", workTools.workerId);
            continue;
        }

        LOG_DEBUG(g_logger, "%d query count: %d", pParam->workerId, workTools.queriedSqlCnt);

        free(data);

        gettimeofday(&timeVal, NULL);
        elapseTime = (timeVal.tv_sec * 1000000 + timeVal.tv_usec) - prevTime;
        LOG_DEBUG(g_logger, "%d work-done in %ld us", pParam->workerId, elapseTime);
    }
    DisconnectPg(workTools.dbWrapper);
    LOG_INFO(g_logger, "%d worker-destroied", pParam->workerId);
}