#ifndef PACKETS_H
#define PACKETS_H

typedef unsigned long long ulonglong;
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
    ulong memFree;           // %lu
    ulong memAvail;          // %lu since Linux 3.14
    ulong memUsed;          // %lu 
    ulong swapFree;         // %lu 
} SMemInfoPacket;   

// #pragma pack(push, 1)
typedef struct SProcInfoPacket
{
    ulong collectTime;
    uint pid;               // %d
    uint ppid;              // %d
    ulong utime;             // %lu  divided by sysconf(_SC_CLK_TCK) => seconds
    ulong stime;             // %lu  divided by sysconf(_SC_CLK_TCK) => seconds
    uchar userName[32];     // %s   max 32
    uchar procName[16];     // %s   max 16
    uchar state;            // %c
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