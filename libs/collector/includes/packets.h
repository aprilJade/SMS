#ifndef PACKETS_H
#define PACKETS_H
#include <stdbool.h>

#define SIGNATURE_CPU        0x534D5363   // SMSc
#define SIGNATURE_AVG_CPU    0x534D5343   // SMSC
#define SIGNATURE_MEM        0x534D536D   // SMSm
#define SIGNATURE_AVG_MEM    0x534D534D   // SMSM
#define SIGNATURE_NET        0x534D536E   // SMSn
#define SIGNATURE_AVG_NET    0x534D534E   // SMSN
#define SIGNATURE_PROC       0x534D5370   // SMSp
#define SIGNATURE_DISK       0x543D5364   // SMSd
#define EXTRACT_SIGNATURE    0x000000FF

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

#pragma pack(push, 1)
typedef struct SHeader
{
    uint signature;
    uchar agentId[16];
    ulong collectTime;
    uint collectPeriod;
    ushort bodyCount;
    ushort bodySize;
} SHeader;

typedef struct SBodyc
{
    ulong usrCpuRunTime;
    ulong sysCpuRunTime;
    ulong idleTime;
    ulong waitTime;
} SBodyc;

typedef struct SBodym
{
    uint memTotal;
    uint memFree;
    uint memAvail;
    uint memUsed;
    uint swapTotal;
    uint swapFree;
} SBodym;

typedef struct SBodyn
{
    uchar nameLength;
    uchar name[15];
    ulong recvBytes;
    ulong recvPackets;
    ulong sendBytes;
    ulong sendPackets;
} SBodyn; 

typedef struct SBodyp
{
    uint pid;
    uint ppid;
    uint utime;
    uint stime;
    uchar procName[16];
    uchar userName[32];
    uchar state;
    ushort cmdlineLen;
} SBodyp;

typedef struct SBodyd
{
    uchar name[16];
    ulong readSuccessCount;
    ulong readSectorCount;
    ulong readTime;
    ulong writeSuccessCount;
    ulong writeSectorCount;
    ulong writeTime;
    uint currentIoCount;
    ulong doingIoTime;
    ulong weightedDoingIoTime;
} SBodyd;

typedef struct SBodyAvgC
{
    float cpuUtilization;   /* Moment of collection */
    float cpuUtilizationAvg;   /* Last hour */
} SBodyAvgC;

typedef struct SbodyAvgM
{
    float memUsage;
    float swapUsage;
    float memUsageAvg;
    float swapUsageAvg;
} SBodyAvgM;

typedef struct SBodyAvgN
{
    uchar nameLength;
    uchar name[15];
    float recvBytesPerSec;
    float recvBytesPerSecAvg;
    float recvPacketsPerSec;
    float recvPacketsPerSecAvg;
    float sendBytesPerSec;
    float sendBytesPerSecAvg;
    float sendPacketsPerSec;
    float sendPacketsPerSecAvg;
} SBodyAvgN;

#pragma pack(pop)

#define UDS_UPDATE_PACKET 0x55445300 // UDS0
#define UDS_STATUS_PACKET 0x55445301 // UDS1

typedef struct SUpdatePacket
{
    char udsPath[108];
    bool bRunCpuCollector;
    unsigned long cpuPeriod;
    bool bRunMemCollector;
    unsigned long memPeriod;
    bool bRunNetCollector;
    unsigned long netPeriod;
    bool bRunProcCollector;
    unsigned long procPeriod;
    bool bRunDiskCOllector;
    unsigned long diskPeriod;
} SUpdatePacket;

typedef struct SAgentStatusPacket
{
    int pid;
    char peerIP[16];
    unsigned short peerPort;
    bool bConnectedWithServer;
    bool bRunCpuCollector;
    unsigned long cpuPeriod;
    bool bRunMemCollector;
    unsigned long memPeriod;
    bool bRunNetCollector;
    unsigned long netPeriod;
    bool bRunProcCollector;
    unsigned long procPeriod;
    bool bRunDiskCOllector;
    unsigned long diskPeriod;
} SAgentStatusPacket;

#endif