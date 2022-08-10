#include "logger.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LOGGER_DEBUG 0
#define TIME_FORMAT "[%02d:%02d:%02d+0900]"

static int CreateDir(char* logPath)
{
#if LOGGER_DEBUG
        printf("MakeDirectory");
#endif
    int idx = 0;
    int dirCnt = 0;
    while (1)
    {
        while (logPath[idx] != '/')
        {
            if (logPath[idx] == '\0')
                break;
            printf("%c\n", logPath[idx]);
            idx++;
        }
        dirCnt++;
        if (logPath[idx++] == '\0')
            break;
    }
#if LOGGER_DEBUG
    printf("dir cnt: %d\n", dirCnt);
#endif
    char buf[260];
    idx = 0;
    for (int i = 0; i < dirCnt; i++)
    {
        while (logPath[idx] != '/')
        {
            if (logPath[idx] == '\0')
                break;
            idx++;
        }
        strncpy(buf, logPath, idx);
        buf[idx] = 0;
#if LOGGER_DEBUG
        printf("%s\n", buf);
#endif
        if (access(buf, F_OK) != 0)
            mkdir(buf, 0777);
        idx++;
    }
}

static int OpenLogFile(char* logPath)
{
    char buf[260];
    time_t localTime;
    struct tm* timeStruct;

    localTime = time(NULL);
    timeStruct = localtime(&localTime);
    if (access(logPath, F_OK) != 0)
        CreateDir(logPath);
        
    sprintf(buf, "%s/%04d-%02d-%02d.log",
        logPath,
        timeStruct->tm_year + 1900,
        timeStruct->tm_mon + 1,
        timeStruct->tm_mday);
    return open(buf, O_WRONLY | O_APPEND | O_CREAT, 0777);
}

void* LoggingRoutine(void* param)
{
    LoggerRoutineParam* pParam = (LoggerRoutineParam*)param;
    char* logMsg;
    
#if LOGGER_DEBUG
    printf("Start logging routine\n");
#endif
    while (1)
    {
        pthread_mutex_lock(pParam->queueLock);
        if (IsEmpty(pParam->queue))
        {
            pthread_mutex_unlock(pParam->queueLock);
            usleep(3000);
            continue;    
        }
        logMsg = (char*)Pop(pParam->queue);
        pthread_mutex_unlock(pParam->queueLock);
#if LOGGER_DEBUG
        printf("Length: %d, Log Message: %s", strlen(logMsg), logMsg);
#endif
        pthread_mutex_lock(pParam->fdLock);
        write(pParam->logFd, logMsg, strlen(logMsg));
        pthread_mutex_unlock(pParam->fdLock);
        free(logMsg);
    }
}

Logger* NewLogger(char* logPath)
{
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    if (logger == NULL)
        return NULL;
    logger->queue = NewQueue();
    LoggerRoutineParam* param = (LoggerRoutineParam*)malloc(sizeof(LoggerRoutineParam));
    if (param == NULL)
    {
        // TODO: handle malloc error
        return NULL;
    }
    // Thread start
    pthread_mutex_init(&logger->queueLock, NULL);
    pthread_mutex_init(&logger->fdLock, NULL);
#if LOGGER_DEBUG
    printf("open log file: %s\n", logPath);
#endif
    if ((param->logFd = OpenLogFile(logPath)) == -1)
    {
        printf("file open failed\n");
        free(logger);
        return NULL;
    }

    param->queue = logger->queue;
    param->fdLock = &logger->fdLock;
    param->queueLock = &logger->queueLock;
    pthread_create(&logger->tid, NULL, LoggingRoutine, param);
    return logger;
}

void DeleteLogger(Logger* logger)
{
    // not implemented yet
}

int Log(Logger* handle, char* logMsg)
{
    assert(handle != NULL);
    time_t localTime;
    struct tm* timeStruct;
    localTime = time(NULL);
    timeStruct = localtime(&localTime);

    int msgLen = strlen(logMsg);
    char* msgBuf = (char*)malloc(sizeof(char) * (msgLen + 24));
    sprintf(msgBuf, "[%02d:%02d:%02d+0900] %s\n", 
        timeStruct->tm_hour,
        timeStruct->tm_min,
        timeStruct->tm_sec,
        logMsg);
#if LOGGER_DEBUG
    printf("%s", msgBuf);
#endif
    pthread_mutex_lock(&handle->queueLock);
    Push(msgBuf, handle->queue);
    pthread_mutex_unlock(&handle->queueLock);
}