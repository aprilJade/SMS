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

#define CONNECTION_COUNT 4
#define HOST "127.0.0.1"
#define PORT 4242

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

    socklen_t len = sizeof(clientAddr);
    void* (*routine)(void*);
    pthread_t tid;
    SReceiveParam* param;

    printf("Start listening: %s:%d\n", HOST, PORT);
    while (1)
    {
        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            perror("server");
            return 1;
        }
        clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
        if (clientFd == -1)
        {
            printf("Fail to connect\n");
            perror("server");
            return 1;
        }
        
        param = (SReceiveParam*)malloc(sizeof(SReceiveParam));
        param->clientSock = clientFd;
    
        if (pthread_create(&tid, NULL, ReceiveRoutine, param) == -1)
        {
            perror("server");
            close(clientFd);
            continue;
        }
        printf("thread created.\n");
        pthread_join(tid, NULL);
    }
}   