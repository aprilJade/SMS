#ifndef PACKETS_H
#define PACKETS_H

typedef unsigned long long ulonglong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef struct SInitialPacket
{
    uint signature;
} SInitialPacket;

typedef struct SCpuInfoPacket
{
    uint signature;
    ulonglong collectTime;
    float usage;
    uint usrCpuRunTime;
    uint sysCpuRunTime;
    uint idleTime;
    uint waitTime;
} SCpuInfoPacket;

typedef struct SMemInfoPacket
{
    uint signature;
    ulonglong collectTime;
    uint memTotal;
    uint memFree;
    uint memAvail;
    uint memUsed;
    uint swapTotal;
    uint swapFree;
} SMemInfoPacket;

#pragma pack(push, 1)
typedef struct SProcInfoPacket
{
    uint signature;
    ulonglong collectTime;
    uint pid;
    uint ppid;
    uint utime;
    uint stime;
    uchar userName[32];
    uchar procName[16];
    uchar state;
    ushort cmdlineLen;
} SProcInfoPacket;
#pragma pack(pop)

typedef struct SNetInfoPacket
{
    uint signature;
    ulonglong collectTime;
    uchar netIfName[16];
    ulonglong recvBytes;
    uint recvPackets;
    ulonglong sendBytes;
    uint sendPackets;
} SNetInfoPacket;

#endif