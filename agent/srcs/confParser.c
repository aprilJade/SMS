#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>

#include "confParser.h"

#define CONF_OPTION_COUNT 14

#define CONF_DEFAULT_ID "debug"
#define CONF_DEFAULT_IP "127.0.0.1"
#define CONF_DEFAULT_PORT 4242
#define CONF_DEAFULT_CPU_PERIOD 3000
#define CONF_DEFAULT_MEM_PREIOD 3000
#define CONF_DEFAULT_NET_PREIOD 3000
#define CONF_DEFAULT_PROC_PREIOD 3000
#define CONF_DEFAULT_DISK_PREIOD 3000
#define CONF_DEFAULT_RUN_DAEMON false


int SetConfDefault(SConf* out)
{
    strcpy(out->agentId, CONF_DEFAULT_ID);
    strcpy(out->hostIp, CONF_DEFAULT_IP);
    out->hostPort = CONF_DEFAULT_PORT;
    out->cpuCollectPeriod = CONF_DEAFULT_CPU_PERIOD;
    out->memCollectPeriod = CONF_DEFAULT_MEM_PREIOD;
    out->netCollectPeriod = CONF_DEFAULT_NET_PREIOD;
    out->procCollectPeriod = CONF_DEFAULT_PROC_PREIOD;
    out->diskCollectPeriod = CONF_DEFAULT_DISK_PREIOD;
    out->bRunAsDaemon = CONF_DEFAULT_RUN_DAEMON;
    return -1;
}

int RemoveSpace(char* s)
{
    int len = strlen(s);
    int i = 0;
    int j = 0; 
    while (s[i])
    {
        j = i;
        while (isspace(s[j]))
            j++;
        if (j != i)
            memmove(&s[i], &s[j], len - j);
        i = j + 1;
    }
}

int MakeConfPair(char* confData, SConfPair* pairs)
{
    char line[128];
    int lineLen = 0;
    int keyLen;
    int valueLen;

    while (*confData)
    {
        // #00. Extract 1 line
        lineLen = 0;
        while (confData[lineLen] != '\n');
            lineLen++;
        if (lineLen == 0)
        {
            confData++;
            continue;
        }
        memcpy(line, confData, lineLen);
        line[lineLen] = 0;
        confData += lineLen + 1;

        RemoveSpace(line);
        lineLen = strlen(line);

        // #01. Parse key
        keyLen = 0;
        while (line[keyLen] != '=' && keyLen < lineLen)
            keyLen++;
        pairs->key = (char*)malloc(sizeof(char) * (keyLen + 1));
        if (pairs->key == NULL)
            return 1;
        strncpy(pairs->key, line, keyLen);
        pairs->key[keyLen] = 0;
        
        if (keyLen == lineLen) // this means there is no value...
        {
            pairs->value = NULL;
            confData++;
            pairs++;
            continue;
        }

        // #02. Parse value
        valueLen = lineLen - keyLen - 1;
        pairs->value = (char*)malloc(sizeof(char) * (valueLen + 1));
        if (pairs->value == NULL)
            return 1;
        strncpy(pairs->value, line + keyLen + 1, valueLen);
        pairs->value[valueLen] = 0;

        pairs++;
    }
    return 0;
}

void ReleaseConfPair(SConfPair* pairs)
{
    for (int i = 0; i < CONF_OPTION_COUNT; i++)
    {
        if (pairs[i].key)
            free(pairs[i].key);
        if (pairs[i].value)
            free(pairs[i].value);
    }
}

int ParseConf(const char* confPath, SConf* out)
{
    char readBuf[1024 + 1] = { 0, };
    int fd = open(confPath, O_RDONLY);
    if (fd == -1)
        return CONF_ERROR_INVALID_PATH;

    int readSize = read(fd, readBuf, 1024);
    if (readSize == -1)
    {
        close(fd);
        return CONF_ERROR_FAIL_READ;
    }   
    readBuf[readSize] = 0;

    if (readBuf[readSize - 1] != '\n')
        return CONF_ERROR_INVALID_FORMAT;

    SetConfDefault(out);

    SConfPair pairs[CONF_OPTION_COUNT] = { 0, };
    if (MakeConfPair(readBuf, pairs) != 0)
    {
        ReleaseConfPair(pairs);
        return CONF_ERROR_MAKE_PAIR;
    }

    for (int i = 0; i < CONF_OPTION_COUNT; i++)
    {
        if (pairs[i].key == NULL)
            break;
        
        if (strcmp(pairs[i].key, "ID") == 0)
        {

        }
        else if (strcmp(pairs[i].key, "HOST_ADDRESS") == 0)
        {

        }
        else if (strcmp(pairs[i].key, "HOST_PORT") == 0)
        {
            
        }
        else if (strcmp(pairs[i].key, "RUN_AS_DAEMON") == 0)
        {
            
        }
        else if (strcmp(pairs[i].key, "RUN_CPU_COLLECTOR") == 0)
        {
            
        }
        else if (strcmp(pairs[i].key, "RUN_MEM_COLLECTOR") == 0)
        {
            
        }
        else if (strcmp(pairs[i].key, "RUN_NET_COLLECTOR") == 0)
        {
            
        }
        else if (strcmp(pairs[i].key, "RUN_PROC_COLLECTOR") == 0)
        {
            
        }
        else if (strcmp(pairs[i].key, "HOST_ADDRESS") == 0)
        {
            
        }
    }

    ReleaseConfPair(pairs);
    return CONF_NO_ERROR;
}
