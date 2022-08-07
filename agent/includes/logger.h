#ifndef LOGGER_H
#define LOGGER_H
#include <pthread.h>

typedef struct Logger
{
    pthread_t pid;
} Logger;

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
    CONN_FAIL_OPT,
    CUR_THRD_CNT_OPT
};

int Log(unsigned char signature, int msg, int protocol, int optionalFlag, int optionValue = 0);

#endif