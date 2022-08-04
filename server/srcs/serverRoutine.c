#include "serverRoutine.h"
#include "../../agent/includes/packets.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define DETAIL_PRINT_CPU 0
#define DETAIL_PRINT_MEM 0
#define DETAIL_PRINT_NET 0
#define DETAIL_PRINT_PROC 0

void initRoutineFuncTable(void **funcTable)
{
    // Add remain routines...
    funcTable[CPU_INFO] = serverCpuInfoRoutine;
    funcTable[MEM_INFO] = serverMemInfoRoutine;
    funcTable[NET_INFO] = serverNetInfoRoutine;
    funcTable[PROC_INFO] = serverProcInfoRoutine;
}

void* serverCpuInfoRoutine(void* param)
{
    size_t receivedPacketCount = 0;
    assert(param != NULL);
    printf("Start serverCpuInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    printf("Read from socket: %d\n", pParam->clientSock);
    int readSize;
    char buf[128] = { 0, };
    SCpuInfoPacket* packet = (SCpuInfoPacket*)buf;
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
        receivedPacketCount++;
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
#else
        printf("cpu packet received count: %ld\n", receivedPacketCount);
#endif
    }
}

void* serverMemInfoRoutine(void* param)
{
    size_t receivedPacketCount = 0;
    assert(param != NULL);    
    printf("Start serverMemInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    printf("Read from socket: %d\n", pParam->clientSock);
    int readSize;
    char buf[128] = { 0, };
    SMemInfoPacket* packet = (SMemInfoPacket*)buf;
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
        receivedPacketCount++;
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
#else
        printf("mem packet received count: %ld\n", receivedPacketCount);
#endif
    }
}

void* serverNetInfoRoutine(void* param)
{
    size_t receivedPacketCount = 0;
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
            return 0;
        }
        if (readSize == 0)
        {
            printf("Received: EOF\n");
            close(pParam->clientSock);
            return 0;
        }
        receivedPacketCount++;
        // TODO: Store to DB
#if DETAIL_PRINT_NET
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
#else
        printf("net packet received count: %ld\n", receivedPacketCount);
#endif
    }
}

void* serverProcInfoRoutine(void* param)
{
    size_t receivedPacketCount = 0;
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
            return 0;
        }
        if (readSize == 0)
        {
            printf("Received: EOF\n");
            close(pParam->clientSock);
            return 0;
        }
        receivedPacketCount++;
        // TODO: Store to DB
#if DETAIL_PRINT_NET
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
#else
        printf("proc packet received count: %ld\n", receivedPacketCount);
#endif
    }
}