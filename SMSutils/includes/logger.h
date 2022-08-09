#ifndef LOGGER_H
#define LOGGER_H
#define KIND_OF_LOG 9
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
    char* host;
    short port;
} Logger;

typedef struct LoggerOptValue
{
    int queueSize;
    int curQueueElemCnt;
    int connFailCnt;
    unsigned long elapseTime;
    int sendBytes;
} LoggerOptValue;

enum eSig
{
    LOG_CPU = 'c',
    LOG_MEMORY = 'm',
    LOG_NETWORK = 'n',
    LOG_PROCESS = 'p'
};

enum eMessage
{
    TRY_CONN,
    FAIL_CONN,
    CONN,
    DISCONN,
    RCV,
    SND,
    QRY,
    THRD_CRT,
    COLL_COMPLETE
};

enum eProtocol
{
    TCP,
    UDP,
    DB,
    SYS
};

enum eOption
{
    NO_OPT,
    DISCONN_OPT,
    CONN_FAIL_OPT,
    COLLECT_ELAPSE_OPT,
    SEND_OPT
};

Logger* NewLogger();
int Log(Logger* handle, char signature, int msg, int protocol, int optionalFlag, void* optionValue);
int SetLoggerParam(Logger* logger, char* host, short port);
void DeleteLogger(Logger* logger);

#endif