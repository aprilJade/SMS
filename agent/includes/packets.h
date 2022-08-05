#ifndef PACKETS_H
#define PACKETS_H

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef struct SInitialPacket
{
    uchar signature[4];
    uint logicalCoreCount;
    uint collectPeriod;
    ulong memTotal;
    ulong swapTotal;
    uchar netIfName[16];
} SInitialPacket;

typedef struct SCpuInfoPacket
{
    ulong collectTime;
    ulong usrCpuRunTime;
    ulong sysCpuRunTime;
    ulong idleTime;
    ulong waitTime;
} SCpuInfoPacket;

typedef struct SMemInfoPacket
{
    ulong collectTime;
    ulong memFree;
    ulong memAvail;
    ulong memUsed;
    ulong swapFree;
    uint collectPeriod;
} SMemInfoPacket;   

// #pragma pack(push, 1)
typedef struct SProcInfoPacket
{
    ulong collectTime;
    uint pid;
    uint ppid;
    ulong utime;
    ulong stime;
    uchar userName[32];
    uchar procName[16];
    uchar state;
    ushort cmdlineLen;
} SProcInfoPacket;
// #pragma pack(pop)

typedef struct SNetInfoPacket
{
    ulong collectTime;
    ulong recvBytes;
    ulong recvPackets;
    ulong sendBytes;
    ulong sendPackets;
} SNetInfoPacket;

#endif