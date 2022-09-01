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
    time_t beginTime;
    unsigned long packetNo;
} SPrefixPkt;

typedef struct SPostfixPkt
{
    char agentId[16];
    char processName[32];
    unsigned int pid;
    unsigned int sendBytes;
    unsigned long elapseTime; 
} SPostfixPkt;
#pragma pack(pop)

#endif