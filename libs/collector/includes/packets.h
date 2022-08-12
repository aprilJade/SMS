#ifndef PACKETS_H
#define PACKETS_H

#define SIGNATURE_CPU     0x534E5363   // SMSc
#define SIGNATURE_MEM     0x534E536D   // SMSm
#define SIGNATURE_NET     0x534E536E   // SMSn
#define SIGNATURE_PROC    0x534E5370   // SMSp
#define EXTRACT_SIGNATURE 0x000000FF

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;


// Normal packet 
#pragma pack(push, 1)
typedef struct SHeader
{
    uint signature;
    ulong collectTime;
    uint collectPeriod;
    ushort bodyCount;
    ushort bodySize;
} SHeader;  // 16

typedef struct SBodyc
{
    ulong usrCpuRunTime;
    ulong sysCpuRunTime;
    ulong idleTime;
    ulong waitTime;
} SBodyc; // 32

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
} SBodyn;   // 48

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
#pragma pack(pop)
// Normal packet end

#endif