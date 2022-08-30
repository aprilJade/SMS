#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

struct SBeforePkt
{
    char agentId[16];
    char processName[32];
    int pid;
    unsigned int hostIp;
    unsigned short hostPort;
};

struct SAfterPkt
{
    char agentId[16];
    char processName[32];
    unsigned int pid;
    unsigned int sendBytes;
    unsigned long elapseTime; 
};

ssize_t send(int fd, const void* buf, size_t len, int flags)
{
    static ssize_t (*orgSend)(int, const void*, size_t, int);
    static bool initialized;
    static int sockFd = 0;
    static struct sockaddr_in sockaddr;
    static struct stat statBuf;
    static struct SBeforePkt beforePkt;
    static struct SAfterPkt afterPkt;
    
    if (initialized == false)
    {
        orgSend = (ssize_t (*)(int, const void*, size_t, int))dlsym(RTLD_NEXT, "send");

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

        // initialize before packet
        beforePkt.pid = getpid();
        char buf[128];
        sprintf(buf, "/proc/%d/stat", beforePkt.pid);
        int fd = open(buf, O_RDONLY);
        if (fd == -1)
            strcpy(beforePkt.processName, "unknown");
        else
        {
            int readSize = read(fd, buf, 128);
            if (readSize < 1)
                strcpy(beforePkt.processName, "unknown");
            else
            {
                buf[readSize] = 0; 
                int i = 0, j = 0;
                while (buf[i++] != ' ');
                i++;
                for (; buf[i] != ')'; i++)
                    beforePkt.processName[j++] = buf[i];            
            }
        }
        initialized = true;
        // beforePkt.processName = 
    }

    //("hooked!: len = %lu\n", len);

    return orgSend(fd, buf, len, flags);
}