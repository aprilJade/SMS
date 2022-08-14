#ifndef COLLECTOR_H
#define COLLECTOR_H

#define BUFFER_SIZE 2048

#include <stddef.h>
#include "packets.h"

typedef struct SCData
{
    uchar* data;
    int dataSize;
} SCData;

enum eCollectorID
{
    CPU = 'c',
    MEMORY = 'm',
    NETWORK = 'n',
    PROCESS = 'p'
};

SCData* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf, int collectPeriod);
SCData* CollectMemInfo(char* buf, int collectPeriod);
SCData* CollectNetInfo(char* buf, int nicCount, int collectPeriod);
SCData* CollectProcInfo(char *buf, uchar* dataBuf, int collectPeriod);

#endif