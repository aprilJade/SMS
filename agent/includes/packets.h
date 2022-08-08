#ifndef PACKETS_H
#define PACKETS_H

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef struct SInitialPacket
{
    uchar signature[4];
    uint collectPeriod;
    uint logicalCoreCount;
    ulong memTotal;
    ulong swapTotal;
    ushort netIFCount;
    ushort isReconnected;
} SInitialPacket;

typedef struct SInitialPacketBody
{
    uchar nameLength;
    uchar name[15];
} SInitialPacketBody;

typedef struct SHeader
{
    uchar signature[4];
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
    uint memFree;
    uint memAvail;
    uint memUsed;
    uint swapFree;
} SBodym;

typedef struct SBodyn
{
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

/*
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

typedef struct SNetInfoPacket
{
    ulong collectTime;
    ulong recvBytes;
    ulong recvPackets;
    ulong sendBytes;
    ulong sendPackets;
} SNetInfoPacket;
*/
#endif