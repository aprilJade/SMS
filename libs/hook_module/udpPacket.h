#ifndef UDP_PACKET_H
#define UDP_PACKET_H

#pragma pack(push, 1)
typedef struct SPrefixPkt
{
    char agentId[16];
    char processName[32];
    int pid;
    char hostIp[16];
    unsigned short hostPort;
    unsigned long packetNo;
} SPrefixPkt;

typedef struct SPostfixPkt
{
    char agentId[16];
    char processName[32];
    unsigned int pid;
    unsigned int maxSendBytes;
    float sendBytesAvg;
    unsigned long maxElapseTime;
    float elapseTimeAvg;
} SPostfixPkt;
#pragma pack(pop)

#endif