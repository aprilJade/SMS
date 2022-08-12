#include "workerRoutine.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

// Calc delta value, average value
// And save to DB
const char* cpuInsertSql = 
    "INSERT INTO cpu_informations(collect_time, core_id, usr_run_time, sys_run_time, idle_time, wait_time) VALUES";
const char* memInsertSql = 
    "INSERT INTO memory_informations(collect_time, total, free, avail, used, swap_total, swap_free) VALUES";
const char* netInsertSql = 
    "INSERT INTO network_informations(collect_time, interface_name, receive_bytes, receive_packets, send_bytes, send_packets) VALUES";
const char* procInsertSql =
    "INSERT INTO process_informations(collect_time, pid, process_name, usr_run_time, sys_run_time, uname, ppid, cmdline) VALUES";

void WorkCpuInfo(void* data, SPgWrapper* wrapper)
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
        if (Query(wrapper, sql) == -1)
            printf("fail query\n");
    }
}

void WorkMemInfo(void* data, SPgWrapper* wrapper)
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
    if (Query(wrapper, sql) == -1)
        printf("fail query\n");
}

void WorkNetInfo(void* data, SPgWrapper* wrapper)
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
        sprintf(sql, "%s (\'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %ld, %ld, %ld, %ld);",
            netInsertSql,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            nicName,
            hBody[i].recvBytes,
            hBody[i].recvPackets,
            hBody[i].sendBytes,
            hBody[i].sendPackets);
        //printf("%s\n", sql);
        if (Query(wrapper, sql) == -1)
            printf("fail query\n");
    }
}

void WorkProcInfo(void* data, SPgWrapper* wrapper)
{
    SHeader* hHeader = (SHeader*)data;
    struct tm* ts;
    ts = localtime(&hHeader->collectTime);
    char sql[1024];
    char sTimestamp[22];
    sprintf(sTimestamp, "\'%04d-%02d-%02d %02d:%02d:%02d\'",
        ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
        ts->tm_hour, ts->tm_min, ts->tm_sec);

    data += sizeof(SHeader);
    SBodyp* hBody;
    char* cmdline;
    for (int i = 0; i < hHeader->bodyCount; i++)
    {
        hBody = (SBodyp*)data;

        data += sizeof(SBodyp);
        if (hBody->cmdlineLen > 0)
        {
            cmdline = (char*)malloc(hBody->cmdlineLen + 1);
            cmdline[hBody->cmdlineLen] = 0;
            data += hBody->cmdlineLen;
            memcpy(cmdline, data, hBody->cmdlineLen);
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
                cmdline);
                free(cmdline);
        }
        else
        {
            sprintf(sql, "%s (%s, %d, \'%s\', \'%c\', %d, %d, %d, \'%s\');",
                procInsertSql,
                sTimestamp,
                hBody->pid,
                hBody->procName,
                hBody->state,
                hBody->ppid,
                hBody->utime,
                hBody->stime,
                hBody->userName);
        }
        if (Query(wrapper, sql) == -1)
            printf("fail query\n");
    }
}

void* WorkerRoutine(void* param)
{
    SWorkerParam* pParam = (SWorkerParam*)param;
    Queue* queue = pParam->queue;
    Logger* logger = pParam->logger;
    char logMsg[128];
    SHeader* hHeader;
    SPgWrapper* pgWrapper = pParam->db;
    void (*work)(void*);
    void* data;

    while (1)
    {
        sprintf(logMsg, "%d worker-ready", pParam->workerId);
        Log(logger, logMsg);
        pthread_mutex_lock(&queue->lock);
        while (IsEmpty(queue))
        {
            pthread_mutex_unlock(&queue->lock);
            usleep(500);
            pthread_mutex_lock(&queue->lock);
        }
        data = Pop(queue);
        pthread_mutex_unlock(&queue->lock);

        sprintf(logMsg, "%d work-start", pParam->workerId);
        Log(logger, logMsg);

        hHeader = (SHeader*)data;
        char pktId = hHeader->signature & EXTRACT_SIGNATURE;
        switch(pktId)
        {
        case 'c':
            WorkCpuInfo(data, pgWrapper);
            break;
        case 'm':
            WorkMemInfo(data, pgWrapper);
            break;
        case 'n':
            WorkNetInfo(data, pgWrapper);
            break;
        case 'p':
            WorkProcInfo(data, pgWrapper);
            break;
        }
        free(data);
        sprintf(logMsg, "%d work-done %c", pParam->workerId, pktId);
        Log(logger, logMsg);
    }
}