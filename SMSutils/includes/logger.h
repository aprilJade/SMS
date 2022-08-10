#ifndef LOGGER_H
#define LOGGER_H
#define SYS_LOG_CNT 10
#define AGENT_LOG_CNT 8
#define SERVER_LOG_CNT 6
#define LOG_BUFFER_SIZE 128
#include <pthread.h>
#include "Queue.h"

typedef struct LoggerBuffer
{
    Queue* queue;
} LoggerBuffer;

typedef struct LoggerRoutineParam
{
    pthread_mutex_t* queueLock;
    pthread_mutex_t* fdLock;
    Queue* queue;
    int logFd;
} LoggerRoutineParam;

typedef struct Logger
{
    pthread_t tid;
    pthread_mutex_t queueLock;
    pthread_mutex_t fdLock;
    Queue* queue;
    const char* logfilePath;
} Logger;

typedef struct LoggerOptValue
{
    int queueSize;
    int curQueueElemCnt;
    int connFailCnt;
    unsigned long elapseTime;
    int sendBytes;
} LoggerOptValue;

Logger* NewLogger(char* logPath);
int Log(Logger* handle, char* logMsg);
void DeleteLogger(Logger* logger);

#endif