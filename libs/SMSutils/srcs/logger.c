#include "logger.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TIME_FORMAT "[%02d:%02d:%02d+0900]"

static const char* const strLogMsg[] = {
    [LOG_FATAL] = "ERROR: FATAL:",
    [LOG_ERROR] = "ERROR:",
    [LOG_INFO] = "INFO:",
    [LOG_DEBUG] = "DEBUG:"
};

static int g_currentDay;

static int CreateDir(char* logPath)
{
    int idx = 0;
    int dirCnt = 0;
    while (1)
    {
        while (logPath[idx] != '/')
        {
            if (logPath[idx] == '\0')
                break;
                idx++;
        }
        dirCnt++;
        if (logPath[idx++] == '\0')
            break;
    }

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

        if (access(buf, F_OK) != 0)
            mkdir(buf, 0777);
        idx++;
    }
}

int GenLogFileFullPath(char* logPath, char* buf)
{
    time_t localTime;
    struct tm* timeStruct;

    localTime = time(NULL);
    timeStruct = localtime(&localTime);
        
    sprintf(buf, "%s/%04d-%02d-%02d.log",
        logPath,
        timeStruct->tm_year + 1900,
        timeStruct->tm_mon + 1,
        timeStruct->tm_mday);
    return 0;
}

static int OpenLogFile(char* logPath)
{
    char buf[260];
    if (access(logPath, F_OK) != 0)
        CreateDir(logPath);
    GenLogFileFullPath(logPath, buf);
    return open(buf, O_CREAT | O_APPEND | O_RDWR, 0777);
}

Logger* NewLogger(char* logPath, int logLevel)
{
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    if (logger == NULL)
        return NULL;
    pthread_mutex_init(&logger->fdLock, NULL);

    if ((logger->logFd = OpenLogFile(logPath)) == -1)
    {
        printf("file open failed\n");
        free(logger);
        return NULL;
    }
    logger->loggingLevel = logLevel;
    logger->logPath = strdup(logPath);
    time_t curTime = time(NULL);
    struct tm* timeStruct = localtime(&curTime);
    g_currentDay = timeStruct->tm_mday;
    return logger;
}


int Log(const Logger* handle, int logLevel, char* logMsg)
{
    assert(handle != NULL && logMsg != NULL);

    if (handle->loggingLevel < logLevel)
        return 0;

    time_t localTime;
    struct tm* timeStruct;
    localTime = time(NULL);
    timeStruct = localtime(&localTime);
    char msgBuf[LOG_BUFFER_SIZE] = { 0, };
    sprintf(msgBuf, "[%02d:%02d:%02d+0900] %s %s\n", 
        timeStruct->tm_hour,
        timeStruct->tm_min,
        timeStruct->tm_sec,
        strLogMsg[logLevel],
        logMsg);

    if (g_currentDay != timeStruct->tm_mday)
    {
        int fd = OpenLogFile(handle->logPath);
        if (fd == -1)
        {
            // TODO: handle error
            
        }
        dup2(handle->logFd, fd);
        close(fd);
        g_currentDay = timeStruct->tm_mday;
    }

    pthread_mutex_lock(&handle->fdLock);
    write(handle->logFd, msgBuf, strlen(msgBuf));
    pthread_mutex_unlock(&handle->fdLock);
}