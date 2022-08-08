#include "serverRoutine.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define DETAIL_PRINT_CPU 0
#define DETAIL_PRINT_MEM 0
#define DETAIL_PRINT_NET 0
#define DETAIL_PRINT_PROC 0

void* ServCpuInfoRoutine(void* param)
{
    assert(param != NULL);
    printf("Start serverCpuInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[512] = { 0, };
    //SCpuInfoPacket* packet = (SCpuInfoPacket*)buf;
    SHeader* hHeader;
    SBodyc* hBody;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 512)) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            return 0;
        }
        
        hHeader = (SHeader*)buf;
        hBody = (SBodyc*)(buf + sizeof(SHeader));

        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            printf("Wait to reconnect....\n");
            return 0;
        }

        if (strncmp(hHeader->signature, "SMSc", 4) != 0 ||
            sizeof(SHeader) + hHeader->bodyCount * hHeader->bodySize != readSize)
        {
            // TODO: Logging
            //printf("Not approved packet.\n");
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Store to DB
        //write(pParam->logFd, "C", 1);
#if DETAIL_PRINT_CPU
        for (int i = 0; i < hHeader->bodyCount; i++)
        {
            printf("%d: %ld %ld %ld %ld\n",
                i, hBody->usrCpuRunTime, hBody->sysCpuRunTime, hBody->idleTime, hBody->waitTime);
            hBody++;
        }
#endif
    }
}

void* ServMemInfoRoutine(void* param)
{
    assert(param != NULL);    
    printf("Start serverMemInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize;
    char buf[512] = { 0, };
    
    SHeader* hHeader;
    SBodym* hBody;

    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 512)) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            return 0;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            printf("Wait to reconnect....\n");
            return 0;
        }

        hHeader = (SHeader*)buf;
        hBody = (SBodym*)(buf + sizeof(SHeader));

        if (strncmp(hHeader->signature, "SMSm", 4) != 0 ||
            sizeof(SHeader) + hHeader->bodyCount * hHeader->bodySize != readSize)
        {
            // TODO: Logging
            //printf("Not approved packet. close SMSm socket\n");
            close(pParam->clientSock);
            return 0;
        }

        // TODO: Store to DB
        //write(pParam->logFd, "m", 1);
#if DETAIL_PRINT_MEM
        printf("%c: %ukB %ukB %ukB %ukB\n",
            hHeader->signature[3],
            hBody->memFree, hBody->memUsed, hBody->memAvail, hBody->swapFree);
#endif
    }
}

void* ServNetInfoRoutine(void* param)
{
    assert(param != NULL);    
    printf("Start serverNetInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize = 0;
    int totalSize = 0;
    char buf[512] = { 0, };
    SHeader* hHeader;
    SBodyn* hBody;

    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 512)) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            printf("Wait to reconnect....\n");
            return 0;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            return 0;
        }
        // TODO: Store to DB
        //write(pParam->logFd, "n", 1);

        hHeader = (SHeader*)buf;
        hBody = (SBodyn*)(buf + sizeof(SHeader));

        if (strncmp(hHeader->signature, "SMSn", 4) != 0 ||
            sizeof(SHeader) + hHeader->bodyCount * hHeader->bodySize != readSize)
        {
            // TODO: Logging
            //printf("Not approved packet. close SMSn socket\n");
            //printf("%c%c%c%c, %d, %d\n",
            //    hHeader->signature[0],
            //    hHeader->signature[1],
            //    hHeader->signature[2],
            //    hHeader->signature[3],
            //    hHeader->bodyCount,
            //    hHeader->bodySize);
            close(pParam->clientSock);
            return 0;
        }
#if DETAIL_PRINT_NET
        for(int i = 0; i < hHeader->bodyCount; i++)
        {
            printf("%c: recv %ld %ld / send %ld %ld\n",
                hHeader->signature[3],
                hBody->recvBytes, hBody->recvPackets, hBody->sendBytes, hBody->sendPackets);
        }
#endif
    }
}

void* ServProcInfoRoutine(void* param)
{
    assert(param != NULL);    
    printf("Start serverProcInfoRoutine\n");
    SServRoutineParam* pParam = (SServRoutineParam*)param;
    int readSize = 0;
    int totalSize = 0;
    char buf[1024 * 1024] = { 0, };
    SHeader* hHeader;
    SBodyn* hBody;

    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 1024 * 1024)) == -1)
        {
            printf("Fail to receive...!\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            free(pParam);
            return 0;
        }
        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            close(pParam->logFd);
            free(pParam);
            printf("Wait to reconnect....\n");
            return 0;
        }
        hHeader = (SHeader*)buf;
        hBody = (SBodyn*)(buf + sizeof(SHeader));

        if (strncmp(hHeader->signature, "SMSp", 4) != 0 ||
            hHeader->bodySize != readSize)
        {
            // TODO: Logging
            //printf("Not approved packet. close SMSp socket. %c%c%c%c %d ? %d\n",
            //    hHeader->signature[0],
            //    hHeader->signature[1],
            //    hHeader->signature[2],
            //    hHeader->signature[3],
            //    readSize, hHeader->bodySize);
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Store to DB
        //write(pParam->logFd, "p", 1);
#if DETAIL_PRINT_PROC
        
#endif
    }
}