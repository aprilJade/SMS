#include "packets.h"
#include "logger.h"
#include "queue.h"
#include "workerUtils.h"
#include <string.h>
#include <stdlib.h>

static const char* const cpuInsertSql = 
    "INSERT INTO cpu_informations(agent_id, collect_time, core_id, usr_run_time, sys_run_time, idle_time, wait_time) VALUES";
static const char* const cpuAvgInsertSql =
    "INSERT INTO cpu_avg_informations(agent_id, collect_time, core_id, cpu_utilization, cpu_utilization_avg) VALUES";
static const char* const memInsertSql = 
    "INSERT INTO memory_informations(agent_id, collect_time, total, free, avail, used, swap_total, swap_free) VALUES";
static const char* const memAvgInsertSql =
    "INSERT INTO memory_avg_informations(agent_id, collect_time, mem_usage, mem_usage_avg, swap_usage, swap_usage_avg) VALUES";
static const char* const netInsertSql = 
    "INSERT INTO network_informations(agent_id, collect_time, interface_name, receive_bytes, receive_packets, send_bytes, send_packets) VALUES";
static const char* const netAvgInsertSql = 
    "INSERT INTO network_avg_informations(agent_id, collect_time, interface_name, receive_bytes_per_sec, receive_bytes_per_sec_avg, receive_packets_per_sec, receive_packets_per_sec_avg, send_bytes_per_sec, send_bytes_per_sec_avg, send_packets_per_sec, send_packets_per_sec_avg) VALUES";
static const char* const procInsertSql =
    "INSERT INTO process_informations(agent_id, collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname, cmdline) VALUES";
static const char* const procInsertSqlNoCmd =
    "INSERT INTO process_informations(agent_id, collect_time, pid, process_name, process_state, ppid, usr_run_time, sys_run_time, uname) VALUES";
static const char* const diskInsertSql =
    "INSERT INTO disk_informations(agent_id, collect_time, mount_point, vfs_type, total_volume, free_volume, usage) VALUES";
static const char* const thresholdInsertSql =
    "INSERT INTO threshold_exceeded_record(agent_id, exceeded_time, threshold_type, threshold_value, exceeded_value) VALUES";

extern Logger* g_logger;

int InsertCpuInfo(char* sqlBuffer, void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyc* hBody;
    struct tm* ts;
    hBody = (SBodyc*)(data + sizeof(SHeader));
    ts = localtime(&hHeader->collectTime);

    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        sprintf(sqlBuffer, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', %d, %ld, %ld, %ld, %ld);",
                    cpuInsertSql,
                    hHeader->agentId,
                    ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                    ts->tm_hour, ts->tm_min, ts->tm_sec,
                    i,
                    hBody[i].usrCpuRunTime,
                    hBody[i].sysCpuRunTime,
                    hBody[i].idleTime,
                    hBody[i].waitTime);
        if (Query(tools->dbWrapper, sqlBuffer) == -1)
        {
            sprintf(sqlBuffer, "%d: Failed to store in DB: CPU", tools->workerId);
            Log(g_logger, LOG_ERROR, sqlBuffer);
            return -1;
        }
        tools->queriedSqlCnt++;
    }
    return 0;
}

int InsertCpuAvgInfo(char* sqlBuffer, void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyAvgC* hBody = (SBodyAvgC*)(data + sizeof(SHeader));
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);
    float totalCpuUtilization = 0;

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
        totalCpuUtilization += hBody[i].cpuUtilization;
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: CPU AVG", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
    }
    totalCpuUtilization /= (float)hHeader->bodyCount;
    if (tools->threshold.cpuUtilization < totalCpuUtilization)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'CPU\', %f, %f);",
            thresholdInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            tools->threshold.cpuUtilization,
            totalCpuUtilization);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: CPU Threshold", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
    }
    tools->queriedSqlCnt++;
    return 0;
}

int InsertMemInfo(char* sqlBuffer, void* data, SWorkTools* tools)
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
    tools->queriedSqlCnt++;
    return 0;
}

int InsertMemAvgInfo(char* sqlBuffer, void* data, SWorkTools* tools)
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
    tools->queriedSqlCnt++;

    if (hBody->memUsage > tools->threshold.memUsage)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'MEM\', %f, %f);",
            thresholdInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            tools->threshold.memUsage,
            hBody->memUsage);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: Mem usage Threshold", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
        tools->queriedSqlCnt++;
    }

    if (hBody->swapUsage > tools->threshold.swapUsage)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'SWAP\', %f, %f);",
            thresholdInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            tools->threshold.swapUsage,
            hBody->swapUsage);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: swap usage Threshold", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
        tools->queriedSqlCnt++;
    }
    return 0;
}

int InsertNetInfo(char* sqlBuffer, void* data, SWorkTools* tools)
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
        tools->queriedSqlCnt++;
    }
    return 0;
}

int InsertNetAvgInfo(char* sqlBuffer, void* data, SWorkTools* tools)
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
            Log(g_logger, LOG_DEBUG, sql);
            sprintf(sql, "%d: Failed to store in DB: Network AVG", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
        
        tools->queriedSqlCnt++;

        if (hBody[i].recvBytesPerSec > tools->threshold.recvBytes)
        {
            sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'RECV\', %lu, %f);",
            thresholdInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            tools->threshold.recvBytes,
            hBody->recvBytesPerSec);
            if (Query(tools->dbWrapper, sql) == -1)
            {
                sprintf(sql, "%d: Failed to store in DB: send bytes threshold", tools->workerId);
                Log(g_logger, LOG_ERROR, sql);
                return -1;
            }
            tools->queriedSqlCnt++;
        }
        if (hBody[i].sendBytesPerSec > tools->threshold.sendBytes)
        {
            sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'SEND\', %lu, %f);",
            thresholdInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            tools->threshold.sendBytes,
            hBody->sendBytesPerSec);
            if (Query(tools->dbWrapper, sql) == -1)
            {
                sprintf(sql, "%d: Failed to store in DB: recv bytes threshold", tools->workerId);
                Log(g_logger, LOG_ERROR, sql);
                return -1;
            }
            tools->queriedSqlCnt++;
        }
    }
    return 0;
}

int InsertProcInfo(char* sqlBuffer, void* data, SWorkTools* tools)
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
        tools->queriedSqlCnt++;
    }

    return 0;
}

int InsertDiskInfo(char* sqlBuffer, void* data, SWorkTools* tools)
{
    SHeader* hHeader = (SHeader*)data;
    SBodyd* hBody = (SBodyd*)(data + sizeof(SHeader));
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);
    char sql[512];

    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', \'%s\', %f, %f, %f);",
            diskInsertSql,
            hHeader->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            hBody->mountPoint, hBody->fsType,
            hBody->totalSizeGB, hBody->freeSizeGB, hBody->diskUsage);
        if (Query(tools->dbWrapper, sql) == -1)
        {
            sprintf(sql, "%d: Failed to store in DB: Disk", tools->workerId);
            Log(g_logger, LOG_ERROR, sql);
            return -1;
        }
        hBody++;
        tools->queriedSqlCnt++;
    }

    return 0;
}

static ulong ConvertToBytesFromLargeUnit(char* str)
{
    ulong result = atol(str);
    int i = 0;
    while(str[i] >= '0' && str[i] <= '9')
        i++;
    
    if (i == strlen(str))
        return result;
    switch (str[i])
    {
    case 'k':
        result *= 1024;
        break;
    case 'M':
        result *= 1024 * 1024;
        break;
    case 'G':
        result *= 1024 * 1024 * 1024;
        break;
    }
    return result;
}

SThreshold GetThresholds(SHashTable* options)
{
    char* value;
    float fTmp;
    uint uTmp;
    SThreshold ret = { 100, 100, 100, 3000, 3000 };

    if ((value = GetValueByKey(CONF_KEY_CPU_UTILIAZATION_THRESHOLD, options)) != NULL)
    {
        fTmp = atof(value);
        if (fTmp >= 0 && fTmp <= 100.0)
            ret.cpuUtilization = fTmp;    
    }

    if ((value = GetValueByKey(CONF_KEY_MEM_USAGE_THRESHOLD, options)) != NULL)
    {  
        fTmp = atof(value);
        if (fTmp >= 0 && fTmp <= 100.0)
            ret.memUsage = fTmp;   
    }

    if ((value = GetValueByKey(CONF_KEY_SWAP_USAGE_THRESHOLD, options)) != NULL)
    {  
        fTmp = atof(value);
        if (fTmp >= 0 && fTmp <= 100.0)
            ret.swapUsage = fTmp;   
    }

    if ((value = GetValueByKey(CONF_KEY_SEND_BYTES_THRESHOLD, options)) != NULL)
    {  
        if (atoi(value) >= 0)
            ret.sendBytes = ConvertToBytesFromLargeUnit(value);   
    }

    if ((value = GetValueByKey(CONF_KEY_RECV_BYTES_THRESHOLD, options)) != NULL)
    {  
        if (atoi(value) >= 0)
            ret.recvBytes = ConvertToBytesFromLargeUnit(value);   
    }

    return ret;
}