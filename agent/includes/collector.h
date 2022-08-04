#ifndef COLLECTOR_H
#define COLLECTOR_H
#define BUFFER_SIZE 512
#include <stddef.h>
#include "packets.h"

void collectEachCpuInfo(long cpuCnt, long timeConversion, char* rdBuf);
void collectCpuInfo(long timeConversion, char* rdBuf, SCpuInfoPacket* packet);
void collectMemInfo(char* buf, SMemInfoPacket* packet);
void collectNetInfo(char* buf, SNetInfoPacket* packet);
// void collectProcInfo(char *buf, size_t maxPid);
void collectProcInfo(char* path, char *buf, SProcInfoPacket* packet);
size_t getMaxPid();

#endif