#ifndef PACKETS_H
#define PACKETS_H

typedef unsigned long long ulonglong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef struct SCpuInfo
{
    uint signature;
    uint collectTime;
    uint usrCpuRunTime;
    uint sysCpuRunTime;
    uint idleTime;
    uint waitTime;
} SCpuInfo;

typedef struct SMemInfo
{
    uint signature;
    uint collectTime;
    uint memTotal;
    uint memFree;
    uint memAvail;
    uint memUsed;
    uint swapTotal;
    uint swapFree;
} SMemInfo;

#pragma pack(push, 1)
typedef struct SProcInfo
{
    uint signature;
    uint collectTime;
    uint pid;
    uint ppid;
    uint utime;
    uint stime;
    uint cutime;
    uint cstime;
    uchar userName[32];
    uchar procName[16];
    ushort cmdlineLen;
} SProcInfo;
#pragma pack(pop)

typedef struct SNetInfo
{
    uint signature;
    uint collectTime;
    uchar netIfName[16];
    ulonglong recvBytes;
    uint recvPackets;
    ulonglong sendBytes;
    uint sendPackets;
} SNetInfo;

#endif