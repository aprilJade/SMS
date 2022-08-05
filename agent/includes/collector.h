#ifndef COLLECTOR_H
#define COLLECTOR_H
#define BUFFER_SIZE 512
#define SIGNATURE_CPU "SMSc"
#define SIGNATURE_MEM "SMSm"
#define SIGNATURE_NET "SMSn"
#define SIGNATURE_PROC "SMSp"
#include <stddef.h>
#include "packets.h"

void collectEachCpuInfo(long cpuCnt, long timeConversion, char* rdBuf);
void collectCpuInfo(long timeConversion, char* rdBuf, SCpuInfoPacket* packet);
void collectMemInfo(char* buf, SMemInfoPacket* packet);
void collectNetInfo(char* buf, SNetInfoPacket* packet);
void collectProcInfo(char* path, char *buf, SProcInfoPacket* packet);

void generateInitialProcPacket(SInitialPacket* packet);
void generateInitialNetPacket(SInitialPacket* packet);
void generateInitialMemPacket(SInitialPacket* packet);
void generateInitialCpuPacket(SInitialPacket* packet);

#endif