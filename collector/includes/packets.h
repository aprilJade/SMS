#ifndef PACKETS_H
#define PACKETS_H

#define SIGNATURE_CPU   0x534E5363   // SMSc
#define SIGNATURE_MEM   0x534E536D   // SMSm
#define SIGNATURE_NET   0x534E536E   // SMSn
#define SIGNATURE_PROC  0x534E5370   // SMSp

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;


// Normal packet 
typedef struct SHeader
{
    uint signature;
    ushort collectPeriod;
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

#pragma pack(push, 1)
typedef struct SBodyp
{
    uint pid;
    uint ppid;
    ulong utime;
    ulong stime;
    uchar procName[16];
    uchar userName[32];
    uchar state;
    ushort cmdlineLen;
} SBodyp;
#pragma pack(pop)
// Normal packet end

#endif