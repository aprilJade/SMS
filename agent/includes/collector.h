#ifndef COLLECTOR_H
#define COLLECTOR_H

#define BUFFER_SIZE 512
#define SIGNATURE_CPU "SMSC"
#define SIGNATURE_MEM "SMSm"
#define SIGNATURE_NET "SMSn"
#define SIGNATURE_PROC "SMSp"

#include <stddef.h>
#include "packets.h"
#include "collectRoutine.h"

enum eCollectorID
{
    CPU = 'C',
    MEMORY = 'm',
    NETWORK = 'n',
    PROCESS = 'p'
};

void CollectEachCpuInfo(long cpuCnt, long timeConversion, char* rdBuf);
void CollectCpuInfo(long timeConversion, char* rdBuf, SCpuInfoPacket* packet);
void CollectMemInfo(char* buf, SMemInfoPacket* packet);
void CollectNetInfo(char* buf, SNetInfoPacket* packet);
void CollectProcInfo(char* path, char *buf, SProcInfoPacket* packet);

void GenerateInitialProcPacket(SInitialPacket* packet, SRoutineParam* param);
void GenerateInitialNetPacket(SInitialPacket* packet, SRoutineParam* param);
void GenerateInitialMemPacket(SInitialPacket* packet, SRoutineParam* param);
void GenerateInitialCpuPacket(SInitialPacket* packet, SRoutineParam* param);

#endif