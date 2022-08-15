#include "hookRoutine.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void* hookRoutine(void* param)
{
    struct sockaddr_in sockaddr;
    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        // TODO: handle error
        perror("agent");
        return -1;        
    }
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddr.sin_port = htons(4848);
    
    while (1)
    {
        //sendto(fd, )
    }
}