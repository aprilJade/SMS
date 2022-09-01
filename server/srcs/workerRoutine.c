#include "workerRoutine.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

extern const Logger* g_logger;
extern Queue* g_queue;

const char* cpuInsertSql = 
    "INSERT INTO cpu_informations(agent_id, collect_time, core_id, usr_run_time, sys_run_time, idle_time, wait_time) VALUES";
const char* memInsertSql = 
    "INSERT INTO memory_informations(agent_id, collect_time, total, free, avail, used, swap_total, swap_free) VALUES";
const char* netInsertSql = 
    "INSERT INTO network_informations(agent_id, collect_time, interface_name, receive_bytes, receive_packets, send_bytes, send_packets) VALUES";
const char* procInsertSql =
    "INSERT INTO process_informations(agent_id, collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname, cmdline) VALUES";
const char* procInsertSqlNoCmd =
    "INSERT INTO process_informations(agent_id, collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname) VALUES";
const char* diskInsertSql =
    "INSERT INTO disk_informations(agent_id, collect_time, device_name, read_success_count, read_sector_count, read_time, write_success_count, write_sector_count, write_time, current_io_count, doing_io_time, weighted_doing_io_time) VALUES";

void InsertCpuInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyc* hBody;
    struct tm* ts;
    hBody = (SBodyc*)(data + sizeof(SHeader));
    ts = localtime(&hHeader->collectTime);

    char sql[512];
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', %d, %ld, %ld, %ld, %ld);",
                    cpuInsertSql,
                    hHeader->agentId,
                    ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                    ts->tm_hour, ts->tm_min, ts->tm_sec,
                    i,
                    hBody[i].usrCpuRunTime,
                    hBody[i].sysCpuRunTime,
                    hBody[i].idleTime,
                    hBody[i].waitTime);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            printf("%s\n", sql);
            sprintf(sql, "%d: Failed to store in DB: CPU", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
        }
    }
}

void InsertCpuAvgInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyAvgC* hBody = (SBodyAvgC*)(data + sizeof(SHeader));

    //for (int i = 0; i < hHeader->bodyCount; i++)
    //    printf("%d: utilization: %.2f avg: %.2f\n", i, hBody[i].cpuUtilization, hBody[i].cpuUtilizationAvg);
}

void InsertMemInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodym* hBody = (SBodym*)(data + sizeof(SHeader));
    struct tm* ts;

    char sql[512];
    ts = localtime(&hHeader->collectTime);
    sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', %d, %d, %d, %d, %d, %d);",
        memInsertSql,
        hHeader->agentId,
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
        sprintf(sql, "%d: Failed to store in DB: Memory", tools->workerId);
        Log(g_logger, LOG_ERROR, sql);
    }
}

void InsertMemAvgInfo(void* data, SWorkTools* tools)
{
    // Not implemented yet
}

void InsertNetInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyn* hBody = (SBodyn*)(data + sizeof(SHeader));
    char nicName[16];
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);

    char sql[512];
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        memcpy(nicName, hBody[i].name, hBody[i].nameLength);
        nicName[hBody[i].nameLength] = 0;
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %ld, %ld, %ld, %ld);",
            netInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            nicName,
            hBody[i].recvBytes,
            hBody[i].recvPackets,
            hBody[i].sendBytes,
            hBody[i].sendPackets);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: Network", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
        }
    }
}

void InsertNetAvgInfo(void* data, SWorkTools* tools)
{
    // Not implemented yet
}

void InsertProcInfo(void* data, SWorkTools* tools)
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

    if (Query(tools->dbWrapper, "BEGIN") == -1)
    {
        sprintf(sql, "%d: Failed to BEGIN command: Process", tools->workerId);
        Log(g_logger, LOG_ERROR, sql);
    }

    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        hBody = (SBodyp*)data;
        data += sizeof(SBodyp);
        if (hBody->cmdlineLen > 0)
        {
            strncpy(cmdlineBuf, data, hBody->cmdlineLen);
            cmdlineBuf[hBody->cmdlineLen] = 0;
            sprintf(sql, "%s (\'%s\', %s, %d, \'%s\', \'%c\', %d, %d, %d, \'%s\', \'%s\');",
                procInsertSql,
                hHeader->agentId,
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
            sprintf(sql, "%s (\'%s\' %s, %d, \'%s\', \'%c\', %d, %d, %d, \'%s\');",
                procInsertSqlNoCmd,
                hHeader->agentId,
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
            sprintf(sql, "%d: Failed to store in DB: Process", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
        }

    }

    if (Query(tools->dbWrapper, "END") == -1)
    {
        sprintf(sql, "%d: Failed to END command: Process", tools->workerId);
        Log(g_logger, LOG_ERROR, sql);
    }
}

void InsertDiskInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyd* hBody = (SBodyd*)(data + sizeof(SHeader));
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);
    char sql[512];

    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %ld, %ld, %ld, %ld, %ld, %ld, %d, %ld, %ld);",
            diskInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            hBody->name,
            hBody->readSuccessCount, hBody->readSectorCount, hBody->readTime,
            hBody->writeSuccessCount, hBody->writeSectorCount, hBody->writeTime,
            hBody->currentIoCount, hBody->doingIoTime, hBody->weightedDoingIoTime);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            printf("%d: %s\n", hHeader->bodyCount, sql);
            sprintf(sql, "%d: Failed to store in DB: Disk", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
        }
        hBody++;
    }
}

static const void (*InsertFunc[])(void*, SWorkTools*) = {
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
    SHeader* hHeader;
    void* data;
    SWorkTools workTools;
    struct timeval timeVal;
    ulong prevTime, postTime, elapseTime, totalElapsed;
    int pktId;
    workTools.dbWrapper = pParam->db;
    workTools.workerId = pParam->workerId;
    
    sprintf(logMsg, "%d worker-created", pParam->workerId);
    Log(g_logger, LOG_INFO, logMsg);
    sprintf(logMsg, "%d work-wait", pParam->workerId);
    Log(g_logger, LOG_DEBUG, logMsg);

    while (1)
    {
        if (IsEmpty(g_queue))
        {
            // TODO: Remove busy wait or change to the optimized set sleep time 
            usleep(500);
            continue;
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
        InsertFunc[hHeader->signature & EXTRACT_SIGNATURE](data, &workTools);
        free(data);

        gettimeofday(&timeVal, NULL);
        elapseTime = (timeVal.tv_sec * 1000000 + timeVal.tv_usec) - prevTime;
        sprintf(logMsg, "%d work-done in %ld us", pParam->workerId, elapseTime);
        Log(g_logger, LOG_DEBUG, logMsg);

        sprintf(logMsg, "%d work-wait", pParam->workerId);
        Log(g_logger, LOG_DEBUG, logMsg);
    }
}