#ifndef COLLECTOR_H
#define COLLECTOR_H
#define BUFFER_SIZE 512
#include <stddef.h>

void collectCpuInfo(long cpuCnt, long timeConversion, char* rdBuf);
void collectMemInfo(char* buf);
void collectNetInfo(char* buf);
size_t getMaxPid();
void collectProcInfo(char *buf, size_t maxPid);

#endif