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

int IsValidSignature(int signature)
{
    if (signature == SIGNATURE_CPU)
        return 1;
    if (signature == SIGNATURE_MEM)
        return 1;
    if (signature == SIGNATURE_NET)
        return 1;
    if (signature == SIGNATURE_PROC)
        return 1;
    return 0;
}

void* ReceiveRoutine(void* param)
{
    printf("Start receive routine.\n");
    SReceiveParam* pParam = (SReceiveParam*)param;
    int readSize;
    char buf[1024 * 1024] = { 0, };

    SHeader* hHeader;
    while (1)
    {
        if ((readSize = read(pParam->clientSock, buf, 1024 * 1024)) == -1)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            printf("Wait to reconnect....\n");
            return 0;
        }
        
        hHeader = (SHeader*)buf;

        if (readSize == 0)
        {
            printf("Disconnected from agent side.\n");
            close(pParam->clientSock);
            printf("Wait to reconnect....\n");
            return 0;
        }

        if (!IsValidSignature(hHeader->signature) ||
            sizeof(SHeader) + hHeader->bodyCount * hHeader->bodySize != readSize)
        {
            // TODO: Logging
            printf("Not approved packet.\n");
            close(pParam->clientSock);
            return 0;
        }
        // TODO: Queuing packet data
        //printf("collect period: %d ms\n", hHeader->collectPeriod);
        //SBodyc* hBody = (SBodyc*)(buf + sizeof(SHeader));
        //for (int i = 0; i < hHeader->bodyCount; i++)
        //{
        //    printf("%d: %012ld %012ld %012ld %012ld\n",
        //        i, hBody[i].usrCpuRunTime, hBody[i].sysCpuRunTime,
        //        hBody[i].idleTime, hBody[i].waitTime);
        //}
        
    }
}