#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "packets.h"
#include <unistd.h>

#define HOST "127.0.0.1"
#define PORT 4242

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
    if (listen(servFd, 10) == -1)
    {
        perror("server");
        close(servFd);
        return 1;
    }
    socklen_t len = sizeof(clientAddr);
    clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
    if (clientFd == -1)
    {
        printf("Fail to connect\n");
        perror("server");
        close(servFd);
        return 1;
    }
    printf("Connected!\n");
    printf("Start communication with agent.\n");
    int readSize;
    char buf[4096] = { 0, };
    SCpuInfoPacket* packet;
    while (1)
    {
        if ((readSize = read(clientFd, buf, 4096)) == -1)
        {
            printf("Fail to receive...!\n");
            close(clientFd);
            close(servFd);
            return 1;
        }
        if (readSize == 0)
        {
            printf("Received: EOF\n");
            close(clientFd);
            close(servFd);
            return 1;
        }
        packet = (SCpuInfoPacket*)buf;
        // TODO: Store to DB
        printf("<Received packet data>\n");
        printf("CPU running time (user mode): %d\n", packet->usrCpuRunTime);
        printf("CPU running time (system mode): %d\n", packet->sysCpuRunTime);
        printf("CPU idle time: %d\n", packet->idleTime);
        printf("CPU I/O wait time: %d\n", packet->waitTime);
        printf("Collcet Time: %lld\n", packet->collectTime);
    }
}