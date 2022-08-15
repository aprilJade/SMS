#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

ssize_t write(int fd, const void* buf, size_t count)
{
    static ssize_t (*orgWrite)(int, const void*, size_t) = 0;
    static int sockFd = 0;
    static struct sockaddr_in sockaddr;

    if (orgWrite == NULL)
        orgWrite = (ssize_t (*)(int, const void*, size_t))dlsym(RTLD_NEXT, "write");

    if (sockFd == 0)
    {
        sockFd = socket(PF_INET, SOCK_DGRAM, 0);
        if (sockFd == -1)
        {
            // TODO: handle error
            perror("agent");
            return -1;
        }
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        sockaddr.sin_port = htons(4848);
    }
    struct stat statBuf;
    fstat(fd, &statBuf);
    if (S_ISSOCK(statBuf.st_mode))
        sendto(sockFd, buf, count, 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

    return orgWrite(fd, buf, count);
}