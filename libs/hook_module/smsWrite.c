#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

ssize_t write(int fd, const void* buf, size_t count)
{
    static ssize_t (*orgWrite)(int, const void*, size_t) = 0;
    static int fd = 0;
    static struct sockaddr_in sockaddr;

    if (orgWrite == NULL)
        orgWrite = (ssize_t (*)(int, const void*, size_t))dlsym(RTLD_NEXT, "write");

    if (fd == 0)
    {
        fd = socket(PF_INET, SOCK_DGRAM, 0);
        if (fd == -1)
        {
            // TODO: handle error
            perror("agent");
            return -1;
        }
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        sockaddr.sin_port = htons(4848);
    }
    char tmp[4];
    memcpy(tmp, buf, 3);
    
    
    sendto(fd, buf, count, 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

    return orgWrite(fd, buf, count);
}