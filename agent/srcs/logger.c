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

static char* logMsgs[KIND_OF_LOG] = {
    "try-connection",   // TRY_CONN
    "fail-connection",  // FAIL_CONN
    "connected",        // CONN
    "disconnected",     // DISCONN
    "received",         // RCV
    "send",             // SND
    "query",            // QRY
    "thread-created"    // THRD_CRT
};

static char* logProtocolStr[3] = {
    "TCP",
    "UDP",
    "DB"
};

// TODO: User could set log directory.
int OpenLogFile()
{
    char buf[260];
    time_t localTime;
    struct tm* timeStruct;

    localTime = time(NULL);
    timeStruct = localtime(&localTime);
    if (access("../log", F_OK) != 0)
        mkdir("../log", 0777);
    if (access("../log/agent", F_OK) != 0)
        mkdir("../log/agent", 0777);

    sprintf(buf, "../log/agent/%04d-%02d-%02d.log",
        timeStruct->tm_year + 1900,
        timeStruct->tm_mon + 1,
        timeStruct->tm_mday);
    int fd = open(buf, O_WRONLY | O_APPEND | O_CREAT, 0777);
    return fd == -1 ? -1 : fd;
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

Logger* NewLogger(char* host, short port)
{
    assert(host != NULL);
    assert(port >= 0 && port <= 65535);
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    logger->host = strdup(host);
    logger->port = port;
    logger->queue = NewQueue();
    LoggerRoutineParam* param = (LoggerRoutineParam*)malloc(sizeof(LoggerRoutineParam));
    // Thread start
    pthread_mutex_init(&logger->queueLock, NULL);
    pthread_mutex_init(&logger->fdLock, NULL);
    if ((param->logFd = OpenLogFile()) == -1)
    {
        printf("file open failed\n");
        free(logger->queue);
        free(logger->host);
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

int Log(Logger* handle, char signature, int msg, int protocol, int optionalFlag, void* optionValue)
{
    assert(handle != NULL);
    assert(optionalFlag <= CONN_FAIL_OPT && optionalFlag >= NO_OPT);
    time_t localTime;
    struct tm* timeStruct;
    LoggerOptValue* optVal = (LoggerOptValue*)optionValue;
    localTime = time(NULL);
    timeStruct = localtime(&localTime);

    // TODO: Think of performance of switch case. Consider to convert from switch case to function pointer table.
    char* msgBuf = (char*)malloc(sizeof(char) * (LOG_BUFFER_SIZE + 1));
    switch (optionalFlag)
    {
    case NO_OPT:
#if LOGGER_DEBUG
        printf("Log NO_OPT\n");
#endif
        sprintf(msgBuf, "[%02d:%02d:%02d+0900] %c %s %s %s:%d\n",
            timeStruct->tm_hour,
            timeStruct->tm_min,
            timeStruct->tm_sec,
            signature,
            logMsgs[msg],
            logProtocolStr[protocol],
            handle->host,
            handle->port);
        break;
    case QUEUE_OPT:
#if LOGGER_DEBUG
        printf("Log QUEUE_OPT\n");
#endif
        sprintf(msgBuf, "[%02d:%02d:%02d+0900] %c %s %s %s:%d %d/%d\n",
            timeStruct->tm_hour,
            timeStruct->tm_min,
            timeStruct->tm_sec,
            signature,
            logMsgs[msg],
            logProtocolStr[protocol],
            handle->host,
            handle->port,
            optVal->curQueueElemCnt,
            optVal->queueSize);
        break;
    case CONN_FAIL_OPT:
#if LOGGER_DEBUG
        printf("Log CONN_FAIL_OPT\n");
#endif
        sprintf(msgBuf, "[%02d:%02d:%02d+0900] %c %s %s %s:%d %d\n",
            timeStruct->tm_hour,
            timeStruct->tm_min,
            timeStruct->tm_sec,
            signature,
            logMsgs[msg],
            logProtocolStr[protocol],
            handle->host,
            handle->port,
            optVal->connFailCnt);
        break;
    default:
        // TODO: handle error...
        break;
    }
#if LOGGER_DEBUG
    printf("%s", msgBuf);
#endif
    pthread_mutex_lock(&handle->queueLock);
    while (IsFull(handle->queue))
    {
        pthread_mutex_unlock(&handle->queueLock);
        usleep(2000);
        pthread_mutex_lock(&handle->queueLock);        
    }
    Push(msgBuf, handle->queue);
    pthread_mutex_unlock(&handle->queueLock);
}