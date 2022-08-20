#ifndef CONF_PARSER_H
#define CONF_PARSER_H

#include "simpleHashTable.h"

enum eConfError // If you want see specific error message, use functions handling errno variable such as "perror"
{
    CONF_NO_ERROR = 0,
    CONF_ERROR_INVALID_PATH,
    CONF_ERROR_FAIL_READ,
    CONF_ERROR_INVALID_FORMAT,
    CONF_ERROR_MAKE_PAIR
};

typedef struct SConf
{
    char agentId[32];
    char hostIp[16];
    unsigned short hostPort;
    bool bRunCpuCollector;
    int cpuCollectPeriod;
    bool bRunMemCollector;
    int memCollectPeriod;
    bool bRunNetCollector;
    int netCollectPeriod;
    bool bRunProcCollector;
    int procCollectPeriod;
    bool bRunDiskCollector;
    int diskCollectPeriod;
    bool bRunAsDaemon;
} SConf;

typedef struct SConfPair
{
    char* key;
    char* value;
} SConfPair;


#endif