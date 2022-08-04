#include "serverRoutine.h"
#include "../../agent/includes/packets.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void initRoutineFuncTable(void **funcTable)
{
    // Add remain routines...
    funcTable[CPU_INFO] = serverCpuInfoRoutine;
    funcTable[MEM_INFO] = serverMemInfoRoutine;
    funcTable[NET_INFO] = serverNetInfoRoutine;
}

void* serverCpuInfoRoutine(void* param)
{
    assert(param != NULL);
    printf("Start serverCpuInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[128] = { 0, };
    SCpuInfoPacket* packet;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SCpuInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 0;
        }
        if (readSize == 0)
        {
            printf("Received: EOF\n");
            close(pParam->clientSock);
            return 0;
        }
        packet = (SCpuInfoPacket*)buf;
        // TODO: Store to DB
        printf("Receive cpu info packet: %d bytes\n", readSize);
    }
}

void* serverMemInfoRoutine(void* param)
{
    assert(param != NULL);    
    printf("Start serverMemInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[128] = { 0, };
    SMemInfoPacket* packet;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SMemInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 0;
        }
        if (readSize == 0)
        {
            printf("Received: EOF\n");
            close(pParam->clientSock);
            return 0;
        }
        packet = (SMemInfoPacket*)buf;
        // TODO: Store to DB
        printf("Receive mem info packet: %d bytes\n", readSize);
    }
}

void* serverNetInfoRoutine(void* param)
{
    assert(param != NULL);    
    printf("Start serverNetInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[128] = { 0, };
    SNetInfoPacket* packet;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SNetInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 0;
        }
        if (readSize == 0)
        {
            printf("Received: EOF\n");
            close(pParam->clientSock);
            return 0;
        }
        packet = (SNetInfoPacket*)buf;
        // TODO: Store to DB
        printf("Receive net info packet: %d bytes\n", readSize);
    }
}
