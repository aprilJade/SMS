#ifndef PACKETS_H
#define PACKETS_H

#define SIGNATURE_CPU     0x534E5363   // SMSc
#define SIGNATURE_MEM     0x534E536D   // SMSm
#define SIGNATURE_NET     0x534E536E   // SMSn
#define SIGNATURE_PROC    0x534E5370   // SMSp
#define SIGNATURE_DISK    0x543E5364   // SMSd
#define EXTRACT_SIGNATURE 0x000000FF

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
    uchar name[16];
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
#pragma pack(pop)

#endif