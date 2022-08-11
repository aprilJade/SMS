#ifndef LOGGER_H
#define LOGGER_H
#define SYS_LOG_CNT 10
#define AGENT_LOG_CNT 8
#define SERVER_LOG_CNT 6
#define LOG_BUFFER_SIZE 128
#include <pthread.h>

typedef struct Logger
{
    pthread_mutex_t fdLock;
    int logFd;
} Logger;

Logger* NewLogger(char* logPath);
int Log(Logger* handle, char* logMsg);
void DeleteLogger(Logger* logger);

#endif