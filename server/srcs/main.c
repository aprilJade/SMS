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
#include <pthread.h>
#include <fcntl.h>
#define HOST "127.0.0.1"
#define PORT 4244
#define CONNECTION_COUNT 1

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

    char buf[128] = { 0, };
    SInitialPacket* packet = (SInitialPacket*)buf;
    socklen_t len = sizeof(clientAddr);
    void* (*routine)(void*);
    pthread_t tid[CONNECTION_COUNT];
    SServRoutineParam* param;
    
    for (int i = 0; i < CONNECTION_COUNT; i++)
    {
        printf("Wait to connect....\n");
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
        case 'C':
            // TODO: Store below data
            // packet->logicalCoreCount;
            routine = ServCpuInfoRoutine;
            break;
        case 'm':
            // TODO: Store below data
            // packet->memTotal;
            // packet->swapTotal;
            routine = ServMemInfoRoutine;
            break;
        case 'p':
            routine = ServProcInfoRoutine;
            break;
        case 'n':
            // TODO: Store below data
            // packet->netIfName;
            routine = ServNetInfoRoutine;
            break;
        default:
            printf("Undefined signature!: %c", packet->signature[3]);
            break;
        }
        param = (SServRoutineParam*)malloc(sizeof(SServRoutineParam));
        param->clientSock = clientFd;
        char buf[16];
        sprintf(buf, "Log-%d", i);
        param->logFd = open(buf, O_CREAT | O_RDWR, 0777);
        if (param->logFd == -1)
        {
            perror("server");
            close(clientFd);
            close(servFd);
            return 0;
        }
        if (pthread_create(&tid[i], NULL, routine, param) == -1)
        {
            perror("server");
            close(clientFd);
            close(param->logFd);
            continue;
        }
    }
    for (int i = 0; i < CONNECTION_COUNT; i++)
        pthread_join(tid[i], NULL);
}   