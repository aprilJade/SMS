#ifndef COLLECTOR_H
#define COLLECTOR_H

#define BUFFER_SIZE 512

#include <stddef.h>
#include "packets.h"

enum eCollectorID
{
    CPU = 'c',
    MEMORY = 'm',
    NETWORK = 'n',
    PROCESS = 'p'
};

uchar* CollectEachCpuInfo(ushort cpuCnt, long timeConversion, char* rdBuf);
uchar* CollectMemInfo(char* buf);
uchar* CollectNetInfo(char* buf, int nicCount);
uchar* CollectProcInfo(char *buf, uchar* dataBuf);


#endif