#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include "serverRoutine.h"
#include "../../agent/includes/packets.h"

#define CONNECTION_COUNT 4
#define HOST "127.0.0.1"
#define PORT 4240

int OpenSocket(char* host, short port)
{
    struct sockaddr_in servAddr;
    int servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
    {
        perror("server");
        return -1;
    }
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(PORT);
    servAddr.sin_family = AF_INET;

    if (bind(servFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
    {
        printf("Fail to bind\n");
        perror("server");
        close(servFd);
        return -1;
    }
    return servFd;
}

int main(void)
{
    printf("Simple SMS server...\n");
    printf("This server just print received data.\n");
    printf("Parse received data and print it.\n");
    printf("When agent implematation is over, this server implemented...!\n");
    
    int servFd, clientFd;
    struct sockaddr_in clientAddr;
    if ((servFd = OpenSocket(HOST, PORT)) == -1)
    {
        perror("server");
        return 1;
    }

    char buf[512] = { 0, };
    SInitialPacket* packet = (SInitialPacket*)buf;
    socklen_t len = sizeof(clientAddr);
    void* (*routine)(void*);
    pthread_t tid;
    SServRoutineParam* param;
    char logPath[32];
    int approved = 0;

    printf("Start listening: %s:%d\n", HOST, PORT);
    while (1)
    {
        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            perror("server");
            close(servFd);
            return 1;
        }
        clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
        if (clientFd == -1)
        {
            printf("Fail to connect\n");
            perror("server");
            close(servFd);
            return 1;
        }
        if (read(clientFd, buf, 512) == -1)
        {
            printf("Fail to receive...!\n");
            close(clientFd);
        }
#if 0
        char str[5] = { 0, };
        strncpy(str, packet->signature, 4);
        printf("%s\n%d\n%d\n%d\n%ld\n%ld\n%d\n",
            str,
            packet->collectPeriod,
            packet->isReconnected,
            packet->logicalCoreCount,
            packet->memTotal,
            packet->swapTotal,
            packet->netIFCount);
#endif
        if (strncmp(packet->signature, "SMS", 3) != 0)
        {
            // TODO: Logging
            printf("Not approved packet signature111. %c%c%c\n",
                packet->signature[0],
                packet->signature[1],
                packet->signature[2]);
            close(clientFd);
            continue;
        }

        switch (packet->signature[3])
        {
        case 'c':
            if (packet->isReconnected)
                break;
            // TODO: Store below data
            // packet->logicalCoreCount;
            routine = ServCpuInfoRoutine;
            sprintf(logPath, "Log-%s", "CPU");
            approved = 1;
            break;
        case 'm':
            if (packet->isReconnected)
                break;
            // TODO: Store below data
            // packet->memTotal;
            // packet->swapTotal;
            routine = ServMemInfoRoutine;
            sprintf(logPath, "Log-%s", "Memory");
            approved = 1;
            break;
        case 'p':
            if (packet->isReconnected)
                break;
            routine = ServProcInfoRoutine;
            sprintf(logPath, "Log-%s", "Process");
            approved = 1;
            break;
        case 'n':
            if (packet->isReconnected)
                break;
            // TODO: Store below data
            // packet->netIfName;
            routine = ServNetInfoRoutine;
            sprintf(logPath, "Log-%s", "Network");
            approved = 1;
            break;
        default:
            printf("Not approved packet signature222. %c\n", packet->signature[3]);
            close(clientFd);
            approved = 0;
            break;
        }
        
        if (!approved)
            continue;

        param = (SServRoutineParam*)malloc(sizeof(SServRoutineParam));
        param->clientSock = clientFd;

        param->logFd = open(logPath, O_CREAT | O_RDWR | O_APPEND, 0777);
        if (param->logFd == -1)
        {
            perror("server");
            close(clientFd);
            close(servFd);
            return 0;
        }
        
        if (pthread_create(&tid, NULL, routine, param) == -1)
        {
            perror("server");
            close(clientFd);
            close(param->logFd);
            continue;
        }
        pthread_detach(tid);
    }
}   