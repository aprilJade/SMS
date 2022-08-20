#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>

#include "confParser.h"

static int RemoveSpace(char* s)
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

int ParseConf(const char* confPath, SHashTable* table)
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

    char* confData = readBuf;
    char* key;
    char* value;
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
        key = (char*)malloc(sizeof(char) * (keyLen + 1));
        if (key == NULL)
            return 1;
        strncpy(key, line, keyLen);
        key[keyLen] = 0;
        
        if (keyLen == lineLen) // this means there is no value...
        {
            AddKeyValue(key, NULL, table);
            free(key);
            continue;
        }

        // #02. Parse value
        valueLen = lineLen - keyLen - 1;
        value = (char*)malloc(sizeof(char) * (valueLen + 1));
        if (value == NULL)
            return 1;
        strncpy(value, line + keyLen + 1, valueLen);
        value[valueLen] = 0;

        AddKeyValue(key, value, table);
        free(key);
        free(value);
    }
    return CONF_NO_ERROR;
}
