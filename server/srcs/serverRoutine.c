#include "serverRoutine.h"
#include "../../agent/includes/packets.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define DETAIL_PRINT_CPU 0
#define DETAIL_PRINT_MEM 0
#define DETAIL_PRINT_NET 1
#define DETAIL_PRINT_PROC 0

int ServCpuInfoRoutine(SServRoutineParam* param)
{
    assert(param != NULL);
    printf("Start serverCpuInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[128] = { 0, };
    SCpuInfoPacket* packet = (SCpuInfoPacket*)buf;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SCpuInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 1;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Store to DB
#if DETAIL_PRINT_CPU
        printf("Receive cpu info packet: %d\n\
                Collect Time: %lld ms\n\
                Running Time (user): %d ms\n\
                Running Time (system): %d ms\n\
                Idle Time: %d ms\n\
                Wait Time: %d ms\n\n",
                readSize, 
                packet->collectTime, 
                packet->usrCpuRunTime,
                packet->sysCpuRunTime,
                packet->idleTime,
                packet->waitTime);
#endif
    }
}

int ServMemInfoRoutine(SServRoutineParam* param)
{
    assert(param != NULL);    
    printf("Start serverMemInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[128] = { 0, };
    SMemInfoPacket* packet = (SMemInfoPacket*)buf;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SMemInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 1;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Store to DB
#if DETAIL_PRINT_MEM
        printf("Receive mem info packet: %d bytes\n\
                Collect Time: %lld ms\n\
                Total Memory: %d kB\n\
                Free Memory: %d kB\n\
                Used Memory: %d kB\n\
                Available Memory: %d kB\n\
                Total Swap Space: %d kB\n\
                Free Swap Space: %d kB\n",
                readSize,
                packet->collectTime,
                packet->memTotal,
                packet->memFree,
                packet->memUsed,
                packet->memAvail,
                packet->swapTotal,
                packet->swapFree);
#endif
    }
}

int ServNetInfoRoutine(SServRoutineParam* param)
{
    assert(param != NULL);    
    printf("Start serverNetInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize = 0;
    int totalSize = 0;
    char buf[128] = { 0, };
    SNetInfoPacket* packet = (SNetInfoPacket*)buf;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SNetInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 1;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Store to DB
#if DETAIL_PRINT_NET
        printf("Receive net info packet: %d bytes\n\
                Collect Time: %ld ms\n\
                Receive Bytes: %ld kB\n\
                Receive Packet Count: %ld\n\
                Send Bytes: %ld kB\n\
                Send Packet Count: %ld\n",
                readSize,
                packet->collectTime,
                packet->recvBytes,
                packet->recvPackets,
                packet->sendBytes,
                packet->sendPackets);
#endif
    }
}

int ServProcInfoRoutine(SServRoutineParam* param)
{
    assert(param != NULL);    
    printf("Start serverProcInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize = 0;
    int totalSize = 0;
    char buf[128] = { 0, };
    SProcInfoPacket* packet = (SProcInfoPacket*)buf;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, sizeof(SProcInfoPacket))) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            return 1;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Store to DB
#if DETAIL_PRINT_PROC
        printf("Receive net info packet: %d bytes\n\
                Collect Time: %lld ms\n\
                Network Interface: %s\n\
                Receive Bytes: %lld B\n\
                Receive Packet Count: %d\n\
                Send Bytes: %lld B\n\
                Send Packet Count: %d\n",
                readSize,
                packet->collectTime,
                packet->netIfName,
                packet->recvBytes,
                packet->recvPackets,
                packet->sendBytes,
                packet->sendPackets);
#endif
    }
}