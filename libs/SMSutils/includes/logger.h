#ifndef LOGGER_H
#define LOGGER_H
#define LOG_BUFFER_SIZE 256
#include <pthread.h>

typedef struct Logger
{
    pthread_mutex_t fdLock;
    char* logPath;
    int loggingLevel;
    int logFd;
} Logger;

Logger* NewLogger(char* logPath, int logLevel);
int Log(const Logger* handle, int logLevel, char* logMsg);
int GenLogFileFullPath(char* logPath, char* buf);

typedef enum eLoggingLevel
{
    LOG_FATAL,
    LOG_ERROR,
    LOG_INFO,   // default
    LOG_DEBUG   
} eLoggingLevel;  

#endif