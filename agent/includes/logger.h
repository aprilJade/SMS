#ifndef LOGGER_H
#define LOGGER_H
#define KIND_OF_LOG 8
#include <pthread.h>
#include "Queue.h"

typedef struct LoggerBuffer
{
    Queue* queue;
} LoggerBuffer;

typedef struct Logger
{
    pthread_t pid;
} Logger;

typedef struct LoggerOptValue
{
    int queueSize;
    int curQueueElemCnt;
    int connFailCnt;
} LoggerOptValue;

enum eSig
{
    CPU = 'C',
    MEMORY = 'm',
    NETWORK = 'n',
    PROCESS = 'p'
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
    THRD_CRT
};

enum eProtocol
{
    TCP,
    UDP,
    DB
};

enum eOption
{
    NO_OPT,
    QUEUE_OPT,
    CONN_FAIL_OPT
};

void InitLogger(Logger* handle);
int Log(Logger* handle, unsigned char signature, int msg, int protocol, int optionalFlag, void* optionValue);

#endif