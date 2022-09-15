#ifndef UDP_PACKET_H
#define UDP_PACKET_H

#define TAGET_SECONDS 60

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
    unsigned long measurementTime;
    unsigned int pid;
    unsigned int sendBytes;
    unsigned int maxSendBytes;
    float sendBytesAvg;
    unsigned long elapseTime;
    unsigned long maxElapseTime;
    float elapseTimeAvg;
} SPostfixPkt;
#pragma pack(pop)

typedef struct SAvgArray 
{
    int prevIdx;
    float elapseAvg[TAGET_SECONDS];
    int elapseCnt[TAGET_SECONDS];
    unsigned long sendBytesAvg[TAGET_SECONDS];
    int sendBytesCnt[TAGET_SECONDS];
} SAvgArray;

#endif