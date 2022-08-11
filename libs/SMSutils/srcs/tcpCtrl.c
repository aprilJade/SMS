#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include "tcpCtrl.h"

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
        return -1;
    return fd;
}

int ParseHost(char* input, char* host, short* port)
{
	int len = 0;
	for (len = 0; input[len] != 0 && input[len] != ':'; len++);
	if (input[len] == '\0')
		return 1;
	strncpy(host, input, len);
	host[len] = 0;	
	*port = atoi(&host[len + 1]);
	return 0;
}