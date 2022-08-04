#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "../../agent/includes/packets.h"
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "serverRoutine.h"

#define HOST "127.0.0.1"
#define PORT 4243
#define CONNECTION_COUNT 3

int main(void)
{
    char* msgs[256];
    msgs[CPU_INFO] = "CPU information";
    msgs[MEM_INFO] = "Memory information";
    msgs[PROC_INFO] = "Process information";
    msgs[NET_INFO] = "Network information";

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
    SInitialPacket* initialPacket;
    void* routineFunctions[256] = { 0, };
    initRoutineFuncTable(routineFunctions);
    void* (*routine)(void*);
    pthread_t tid[CONNECTION_COUNT];
    int readSize;
    char buf[128] = { 0, };

    for (int i = 0; i < CONNECTION_COUNT; i++)
    {
        if (listen(servFd, 42) == -1)
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
        if ((readSize = read(clientFd, buf, 128)) == -1)
        {
            printf("Fail to receive...!\n");
            close(clientFd);
        }
        SInitialPacket* packet = (SInitialPacket*)buf;
        char* signature = (char*)&packet->signature;
        routine = routineFunctions[signature[3]];
        if (routine == NULL)
        {
            printf("Undefined Signature!!: ");
            write(1, signature, 4);
            putchar('\n');
            continue;
        }
        printf("%d: Connected for accept %s.\n", i, msgs[signature[3]]);
        SServRoutineParam param;
        param.clientSock = clientFd;
        pthread_create(&tid[i], NULL, routine, &param);
    }
    for (int i = 0; i < CONNECTION_COUNT; i++)
        pthread_join(tid[i], NULL);
}   