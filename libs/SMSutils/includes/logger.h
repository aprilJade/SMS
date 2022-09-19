#ifndef LOGGER_H
#define LOGGER_H
#define LOG_BUFFER_SIZE 512
#include <pthread.h>

#define LOG_DEBUG(x, ...) Log(x, LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_INFO(x, ...) Log(x, LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_ERROR(x, ...) Log(x, LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define LOG_FATAL(x, ...) Log(x, LOG_LEVEL_FATAL, __FILE__, __func__, __LINE__, __VA_ARGS__);

typedef struct Logger
{
    pthread_mutex_t fdLock;
    char* logPath;
    int loggingLevel;
    int logFd;
} Logger;

Logger* NewLogger(char* logPath, int logLevel);
int Log(const Logger* handle, int logLevel,
    const char* fileName, const char* funcName, int lineNo, char* fmt, ...);
int GenLogFileFullPath(char* logPath, char* buf);

typedef enum eLoggingLevel
{
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,   // default
    LOG_LEVEL_DEBUG   
} eLoggingLevel;  

#endif