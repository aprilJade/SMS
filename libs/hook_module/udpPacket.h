#ifndef UDP_PACKET_H
#define UDP_PACKET_H

#pragma pack(push, 1)
typedef struct SPrefixPkt
{
    char agentId[16];
    char processName[32];
    int pid;
    in_addr_t hostIp;
    in_port_t hostPort;
    time_t beginTime;
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