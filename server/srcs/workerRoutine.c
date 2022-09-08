#include "workerRoutine.h"
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

const char* cpuInsertSql = 
    "INSERT INTO cpu_informations(agent_id, collect_time, core_id, usr_run_time, sys_run_time, idle_time, wait_time) VALUES";
const char* cpuAvgInsertSql =
    "INSERT INTO cpu_avg_informations(agent_id, collect_time, core_id, cpu_utilization, cpu_utilization_avg) VALUES";
const char* memInsertSql = 
    "INSERT INTO memory_informations(agent_id, collect_time, total, free, avail, used, swap_total, swap_free) VALUES";
const char* memAvgInsertSql =
    "INSERT INTO memory_avg_informations(agent_id, collect_time, mem_usage, mem_usage_avg, swap_usage, swap_usage_avg) VALUES";
const char* netInsertSql = 
    "INSERT INTO network_informations(agent_id, collect_time, interface_name, receive_bytes, receive_packets, send_bytes, send_packets) VALUES";
const char* netAvgInsertSql = 
    "INSERT INTO network_avg_informations(agent_id, collect_time, interface_name, receive_bytes_per_sec, receive_bytes_per_sec_avg, receive_packets_per_sec, receive_packets_per_sec_avg, send_bytes_per_sec, send_bytes_per_sec_avg, send_packets_per_sec, send_packets_per_sec_avg) VALUES";
const char* procInsertSql =
    "INSERT INTO process_informations(agent_id, collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname, cmdline) VALUES";
const char* procInsertSqlNoCmd =
    "INSERT INTO process_informations(agent_id, collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname) VALUES";
const char* diskInsertSql =
    "INSERT INTO disk_informations(agent_id, collect_time, device_name, read_success_count, read_sector_count, read_time, write_success_count, write_sector_count, write_time, current_io_count, doing_io_time, weighted_doing_io_time) VALUES";

int InsertCpuInfo(void* data, SWorkTools* tools)
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
            sprintf(sql, "%d: Failed to store in DB: CPU", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
    }
    return 0;
}

int InsertCpuAvgInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyAvgC* hBody = (SBodyAvgC*)(data + sizeof(SHeader));
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);

    char sql[512];
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', %d, %f, %f);",
                    cpuAvgInsertSql,
                    hHeader->agentId,
                    ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                    ts->tm_hour, ts->tm_min, ts->tm_sec,
                    i,
                    hBody[i].cpuUtilization,
                    hBody[i].cpuUtilizationAvg);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: CPU AVG", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
    }
    return 0;
}

int InsertMemInfo(void* data, SWorkTools* tools)
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
        return -1;
    }
    return 0;
}

int InsertMemAvgInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyAvgM* hBody = (SBodyAvgM*)(data + sizeof(SHeader));
    struct tm* ts;

    char sql[512];
    ts = localtime(&hHeader->collectTime);
    sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', %f, %f, %f, %f);",
        memAvgInsertSql,
        hHeader->agentId,
        ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
        ts->tm_hour, ts->tm_min, ts->tm_sec,
        hBody->memUsage,
        hBody->memUsageAvg,
        hBody->swapUsage,
        hBody->swapUsageAvg);
    if (Query(tools->dbWrapper, sql) == -1)
    {
        sprintf(sql, "%d: Failed to store in DB: Memory AVG", tools->workerId);
        Log(g_logger, LOG_ERROR, sql);
        return -1;
    }
    return 0;
}

int InsertNetInfo(void* data, SWorkTools* tools)
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
            return -1;
        }
    }
    return 0;
}

int InsertNetAvgInfo(void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyAvgN* hBody = (SBodyAvgN*)(data + sizeof(SHeader));
    char nicName[16];
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);

    char sql[512];
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        memcpy(nicName, hBody[i].name, hBody[i].nameLength);
        nicName[hBody[i].nameLength] = 0;
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %f, %f, %f, %f, %f, %f, %f, %f);",
            netAvgInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            nicName,
            hBody[i].recvBytesPerSec,
            hBody[i].recvBytesPerSecAvg,
            hBody[i].recvPacketsPerSec,
            hBody[i].recvPacketsPerSecAvg,
            hBody[i].sendBytesPerSec,
            hBody[i].sendBytesPerSecAvg,
            hBody[i].sendPacketsPerSec,
            hBody[i].sendPacketsPerSecAvg);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: Network AVG", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
    }
    return 0;
}

int InsertProcInfo(void* data, SWorkTools* tools)
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
        return -1;
    }
    return 0;
}

int InsertDiskInfo(void* data, SWorkTools* tools)
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
            sprintf(sql, "%d: Failed to store in DB: Disk", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
        hBody++;
    }
    return 0;
}

static const int (*InsertFunc[])(void*, SWorkTools*) = {
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
    workTools.workerId = pParam->workerId;
    workTools.dbWrapper = pParam->db;
    
    sprintf(logMsg, "%d worker-created", pParam->workerId);
    Log(g_logger, LOG_INFO, logMsg);
    sprintf(logMsg, "%d work-wait", pParam->workerId);
    Log(g_logger, LOG_DEBUG, logMsg);

    while (1)
    {
        if (g_turnOff)
            break;
        if (!pParam->db->connected)
        {
            sprintf(logMsg, "%d Go to sleep: DB connection is bad", pParam->workerId);
            Log(g_logger, LOG_FATAL, logMsg);
            pthread_mutex_lock(&pParam->db->lock);
            pthread_cond_wait(&pParam->db->cond, &pParam->db->lock);
            pthread_mutex_unlock(&pParam->db->lock);
            sprintf(logMsg, "%d Ready to work: DB connection recovered", pParam->workerId);
            Log(g_logger, LOG_INFO, logMsg);
            continue;
        }

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
                sprintf(logMsg, "%d Wakeup: New client connected", pParam->workerId);
                Log(g_logger, LOG_INFO, logMsg);
            }
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
        if (InsertFunc[hHeader->signature & EXTRACT_SIGNATURE](data, &workTools) == -1)
            pParam->db->connected = false;

        free(data);

        gettimeofday(&timeVal, NULL);
        elapseTime = (timeVal.tv_sec * 1000000 + timeVal.tv_usec) - prevTime;
        sprintf(logMsg, "%d work-done in %ld us", pParam->workerId, elapseTime);
        Log(g_logger, LOG_DEBUG, logMsg);

        sprintf(logMsg, "%d work-wait", pParam->workerId);
        Log(g_logger, LOG_DEBUG, logMsg);
    }

    free(data);
    sprintf(logMsg, "%d worker-destroied", pParam->workerId);
    Log(g_logger, LOG_INFO, logMsg);
}