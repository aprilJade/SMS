#ifndef COLLECTOR_H
#define COLLECTOR_H
#define BUFFER_SIZE 512
#include <stddef.h>
#include "packets.h"

void collectEachCpuInfo(long cpuCnt, long timeConversion, char* rdBuf);
void collectCpuInfo(long timeConversion, char* rdBuf, SCpuInfoPacket* packet);
void collectMemInfo(char* buf);
void collectNetInfo(char* buf);
void collectProcInfo(char *buf, size_t maxPid);
size_t getMaxPid();

#endif