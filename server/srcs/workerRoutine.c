#include "workerRoutine.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

// Calc delta value, average value
// And save to DB
const char* cpuInsertSql = 
    "INSERT INTO cpu_informations(collect_time, core_id, usr_run_time, sys_run_time, idle_time, wait_time) VALUES";
const char* memInsertSql = 
    "INSERT INTO memory_informations(collect_time, total, free, avail, used, swap_total, swap_free) VALUES";
const char* netInsertSql = 
    "INSERT INTO network_informations(collect_time, interface_name, receive_bytes, receive_packets, send_bytes, send_packets) VALUES";
const char* procInsertSql =
    "INSERT INTO process_informations(collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname, cmdline) VALUES";
const char* procInsertSqlNoCmd =
    "INSERT INTO process_informations(collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname) VALUES";

void WorkCpuInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyc* hBody;
    struct tm* ts;
    hBody = (SBodyc*)(data + sizeof(SHeader));
    ts = localtime(&hHeader->collectTime);

    char sql[512];
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        sprintf(sql, "%s (\'%04d-%02d-%02d %02d:%02d:%02d\', %d, %ld, %ld, %ld, %ld);",
                    cpuInsertSql,
                    ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                    ts->tm_hour, ts->tm_min, ts->tm_sec,
                    i,
                    hBody[i].usrCpuRunTime,
                    hBody[i].sysCpuRunTime,
                    hBody[i].idleTime,
                    hBody[i].waitTime);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "ERROR: %d: Failed to store in DB: CPU", tools->workerId);
            Log(tools->logger, sql);
        }
    }
}

void WorkMemInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodym* hBody = (SBodym*)(data + sizeof(SHeader));
    struct tm* ts;

    char sql[512];
    ts = localtime(&hHeader->collectTime);
    sprintf(sql, "%s (\'%04d-%02d-%02d %02d:%02d:%02d\', %d, %d, %d, %d, %d, %d);",
        memInsertSql,
        ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
        ts->tm_hour, ts->tm_min, ts->tm_sec,
        hBody->memTotal,
        hBody->memFree,
        hBody->memUsed,
        hBody->memAvail,
        hBody->swapTotal,
        hBody->swapFree);
    if (Query(tools->dbWrapper, sql) == -1)
    {
        sprintf(sql, "ERROR: %d: Failed to store in DB: Memory", tools->workerId);
        Log(tools->logger, sql);
    }
}

void WorkNetInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyn* hBody = (SBodyn*)(data + sizeof(SHeader));
    char nicName[16];
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);

    char sql[512];
    printf("process count: %d\n", hHeader->bodyCount);
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        memcpy(nicName, hBody[i].name, hBody[i].nameLength);
        nicName[hBody[i].nameLength] = 0;
        sprintf(sql, "%s (\'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %ld, %ld, %ld, %ld);",
            netInsertSql,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            nicName,
            hBody[i].recvBytes,
            hBody[i].recvPackets,
            hBody[i].sendBytes,
            hBody[i].sendPackets);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "ERROR: %d: Failed to store in DB: Network", tools->workerId);
            Log(tools->logger, sql);
        }
    }
}

void WorkProcInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);
    char sql[4096];
    char sTimestamp[22];
    sprintf(sTimestamp, "\'%04d-%02d-%02d %02d:%02d:%02d\'",
        ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
        ts->tm_hour, ts->tm_min, ts->tm_sec);

    data += sizeof(SHeader);
    SBodyp* hBody;
    char cmdlineBuf[2048];

    // Start transaction block
    if (Query(tools->dbWrapper, "BEGIN") == -1)
    {
        sprintf(sql, "ERROR: %d: Failed to BEGIN command: Process", tools->workerId);
        Log(tools->logger, sql);
    }

    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        hBody = (SBodyp*)data;
        data += sizeof(SBodyp);
        if (hBody->cmdlineLen > 0)
        {
            strncpy(cmdlineBuf, data, hBody->cmdlineLen);
            cmdlineBuf[hBody->cmdlineLen] = 0;
            sprintf(sql, "%s (%s, %d, \'%s\', \'%c\', %d, %d, %d, \'%s\', \'%s\');",
                procInsertSql,
                sTimestamp,
                hBody->pid,
                hBody->procName,
                hBody->state,
                hBody->ppid,
                hBody->utime,
                hBody->stime,
                hBody->userName,
                cmdlineBuf);
            data += hBody->cmdlineLen;
        }
        else
        {
            sprintf(sql, "%s (%s, %d, \'%s\', \'%c\', %d, %d, %d, \'%s\');",
                procInsertSqlNoCmd,
                sTimestamp,
                hBody->pid,
                hBody->procName,
                hBody->state,
                hBody->ppid,
                hBody->utime,
                hBody->stime,
                hBody->userName);
        }

        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "ERROR: %d: Failed to store in DB: Process", tools->workerId);
            Log(tools->logger, sql);
        }

    }
    
    // Commit current transaction block
    if (Query(tools->dbWrapper, "END") == -1)
    {
        sprintf(sql, "ERROR: %d: Failed to END command: Process", tools->workerId);
        Log(tools->logger, sql);
    }
}

void* WorkerRoutine(void* param)
{
    SWorkerParam* pParam = (SWorkerParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    char logMsg[128];
    SHeader* hHeader;
    void* data;
    SWorkTools workTools;
    struct timeval timeVal;
    ulong prevTime, postTime, elapseTime, totalElapsed;

    workTools.dbWrapper = pParam->db;
    workTools.workerId = pParam->workerId;
    workTools.logger = logger;

    while (1)
    {
        pthread_mutex_lock(&queue->lock);
        if (IsEmpty(queue))
        {
            pthread_mutex_unlock(&queue->lock);

            sprintf(logMsg, "TRACE: %d wait for work", pParam->workerId);
            Log(logger, logMsg);
            
            pthread_mutex_lock(&queue->lock);
            while (IsEmpty(queue))
            {
                pthread_mutex_unlock(&queue->lock);
                usleep(500);
                pthread_mutex_lock(&queue->lock);
            }

            sprintf(logMsg, "TRACE: %d start work", pParam->workerId);
            Log(logger, logMsg);
        }
        data = Pop(queue);
        pthread_mutex_unlock(&queue->lock);
        

        gettimeofday(&timeVal, NULL);
        prevTime = timeVal.tv_sec * 1000000 + timeVal.tv_usec;

        hHeader = (SHeader*)data;
        char pktId = hHeader->signature & EXTRACT_SIGNATURE;
        switch(pktId)
        {
        case 'c':
            WorkCpuInfo(data, &workTools);
            break;
        case 'm':
            WorkMemInfo(data, &workTools);
            break;
        case 'n':
            WorkNetInfo(data, &workTools);
            break;
        case 'p':
            WorkProcInfo(data, &workTools);
            break;
        }
        free(data);

        gettimeofday(&timeVal, NULL);
        elapseTime = (timeVal.tv_sec * 1000000 + timeVal.tv_usec) - prevTime;
        sprintf(logMsg, "TRACE: %d work-done in %ld us", pParam->workerId, elapseTime);
        Log(logger, logMsg);
    }
}