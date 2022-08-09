#ifndef SERVER_ROUTINE_H
#define SERVER_ROUTINE_H
#define FUNC_TABLE_SIZE 256
#include "packets.h"

typedef struct SReceiveParam
{
    int clientSock;
} SReceiveParam;

void* ReceiveRoutine(void* param);

void* ServCpuInfoRoutine(void* param);
void* ServMemInfoRoutine(void* param);
void* ServNetInfoRoutine(void* param);
void* ServProcInfoRoutine(void* param);

#endif