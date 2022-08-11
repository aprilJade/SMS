#ifndef SERVER_ROUTINE_H
#define SERVER_ROUTINE_H
#define FUNC_TABLE_SIZE 256
#include "packets.h"
#include "logger.h"
#include "Queue.h"

typedef struct SReceiveParam
{
    int clientSock;
    Logger* logger;
    Queue* queue;
    char* host;
    short port;
} SReceiveParam;

void* ReceiveRoutine(void* param);

#endif