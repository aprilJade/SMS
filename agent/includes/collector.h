#ifndef COLLECTOR_H
#define COLLECTOR_H
#define BUFFER_SIZE 512
#define SIGNATURE_CPU "SMSc"
#define SIGNATURE_MEM "SMSm"
#define SIGNATURE_NET "SMSn"
#define SIGNATURE_PROC "SMSp"
#include <stddef.h>
#include "packets.h"

void CollectEachCpuInfo(long cpuCnt, long timeConversion, char* rdBuf);
void CollectCpuInfo(long timeConversion, char* rdBuf, SCpuInfoPacket* packet);
void CollectMemInfo(char* buf, SMemInfoPacket* packet);
void CollectNetInfo(char* buf, SNetInfoPacket* packet);
void CollectProcInfo(char* path, char *buf, SProcInfoPacket* packet);

void GenerateInitialProcPacket(SInitialPacket* packet);
void GenerateInitialNetPacket(SInitialPacket* packet);
void GenerateInitialMemPacket(SInitialPacket* packet);
void GenerateInitialCpuPacket(SInitialPacket* packet);

#endif