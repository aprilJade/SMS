#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include "../../agent/includes/packets.h"
#include <unistd.h>
#include <assert.h>
#include "serverRoutine.h"
#include <stdlib.h>
#define HOST "127.0.0.1"
#define PORT 4243
#define CONNECTION_COUNT 3

int main(void)
{
    printf("Simple SMS server...\n");
    printf("This server just print received data.\n");
    printf("Parse received data and print it.\n");
    printf("When agent implematation is over, this server implemented...!\n");
    int servFd, clientFd;

    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
    {
        perror("server");
        return (1);
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
        return 1;
    }
    printf("Wait to connect....\n");
    socklen_t len = sizeof(clientAddr);
    int (*routine)(SServRoutineParam*);
    char buf[128] = { 0, };
    SInitialPacket* packet = (SInitialPacket*)buf;
    pid_t pid[CONNECTION_COUNT];
    for (int i = 0; i < CONNECTION_COUNT; i++)
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
        if (read(clientFd, buf, 128) == -1)
        {
            printf("Fail to receive...!\n");
            close(clientFd);
        }
        switch (packet->signature[3])
        {
        case 'c':
            // TODO: Store below data
            // packet->logicalCoreCount;
            routine = serverCpuInfoRoutine;
            break;
        case 'm':
            // TODO: Store below data
            // packet->memTotal;
            // packet->swapTotal;
            routine = serverMemInfoRoutine;
            break;
        case 'p':
            routine = serverProcInfoRoutine;
            break;
        case 'n':
            // TODO: Store below data
            // packet->netIfName;
            routine = serverNetInfoRoutine;
            break;
        default:
            printf("Undefined signature!: %c", packet->signature[3]);
            break;
        }
        SServRoutineParam param;
        param.clientSock = clientFd;
        pid[i] = fork();
        if (pid[i] == -1)
        {
            perror("server");
            return 1;
        }
        if (pid[i] == 0)
            exit(routine(&param));
        else
            close(clientFd);
    }
    for (int i = 0; i < CONNECTION_COUNT; i++)
        wait(&pid[i]);
}   