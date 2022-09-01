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
#include <time.h>

#pragma pack(push, 1)
struct SPrefixPkt
{
    char agentId[16];
    char processName[32];
    int pid;
    in_addr_t hostIp;
    in_port_t hostPort;
    time_t beginTime;
};

struct SPostfixPkt
{
    char agentId[16];
    char processName[32];
    unsigned int pid;
    unsigned int sendBytes;
    unsigned long elapseTime; 
};
#pragma pack(pop)

static const char* defaultHostIp = "127.0.0.1";
static const unsigned short defaultHostPort = 4343;
static ssize_t (*orgSend)(int, const void*, size_t, int);
static int sockFd;
static struct sockaddr_in servAddr;
static socklen_t sockLen;
static struct stat statBuf;
static struct SPrefixPkt prevPkt;
static size_t prePktSize;
static struct SPostfixPkt postPkt;
static size_t postPktSize;

static void InitializeUdpPacket(struct SPrefixPkt* prevPkt, struct SPostfixPkt* postPkt)
{
    /* Initialize Prefix Packet */
    prevPkt->pid = getpid();
    char buf[128];
    sprintf(buf, "/proc/%d/stat", prevPkt->pid);
    int fd = open(buf, O_RDONLY);
    if (fd == -1)
        strcpy(prevPkt->processName, "unknown");
    else
    {
        int readSize = read(fd, buf, 128);
        if (readSize < 1)
            strcpy(prevPkt->processName, "unknown");
        else
        {
            buf[readSize] = 0; 
            int i = 0, j = 0;
            while (buf[i++] != ' ');
            i++;
            for (; buf[i] != ')'; i++)
                prevPkt->processName[j++] = buf[i];            
        }
    }
    strcpy(prevPkt->agentId, "udp-test");
    prevPkt->beginTime = time(NULL);
    prevPkt->hostIp = inet_addr(defaultHostIp);
    prevPkt->hostPort = htons(defaultHostPort);
    prePktSize = sizeof(prevPkt);

    /* Initialize Postfix Packet */
    strcpy(postPkt->agentId, prevPkt->agentId);
    strcpy(postPkt->processName, prevPkt->processName);
    postPkt->pid = prevPkt->pid;
    postPktSize = sizeof(postPkt);
}

__attribute__((constructor))
static void InitializeHookingModule()
{
    printf("module loaded\n");
    orgSend = (ssize_t (*)(int, const void*, size_t, int))dlsym(RTLD_NEXT, "send");
    sockFd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockFd == -1)
    {
        // TODO: handle error
        perror("agent");
        return -1;
    }
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(defaultHostIp);
    servAddr.sin_port = htons(defaultHostPort);
    sockLen = sizeof(servAddr);
    InitializeUdpPacket(&prevPkt, &postPkt);
}

ssize_t send(int fd, const void* buf, size_t len, int flags)
{   
    int sendBytes;
    
    sendto(sockFd, &prevPkt, prePktSize, 0, (struct sockaddr*)&servAddr, sockLen);

    if ((sendBytes = sendto(sockFd, buf, len, 0, (struct sockaddr*)&servAddr, sockLen)) < 0)
        printf("fail to udp send\n");

    postPkt.elapseTime = time(NULL) - prevPkt.beginTime;
    postPkt.sendBytes = sendBytes;
    sendto(sockFd, &postPkt, prePktSize, 0, (struct sockaddr*)&servAddr, sockLen);

    return orgSend(fd, buf, len, flags);
}