#ifndef COLLECTOR_H
#define COLLECTOR_H

#define BUFFER_SIZE 512
#define SIGNATURE_CPU "SMSc"
#define SIGNATURE_MEM "SMSm"
#define SIGNATURE_NET "SMSn"
#define SIGNATURE_PROC "SMSp"

#include <stddef.h>
#include "packets.h"
#include "routines.h"

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