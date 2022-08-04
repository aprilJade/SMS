#include "tcpCtrl.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>

int ConnectToServer(char *host, short port)
{
    assert(host && port >= 0 && port < 65536);
    struct sockaddr_in sockaddr;
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        // TODO: handle error
        perror("agent");
        return -1;        
    }
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(host);
    sockaddr.sin_port = htons(port);
    if (connect(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
    {
        // TODO: handle error
        perror("agent");
        return -1;
    }
    else
    {
        // TODO: convert printf to log
        printf("Success connecting to server\n");
    }
    return fd;
}