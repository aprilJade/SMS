#ifndef SERVER_ROUTINE_H
#define SERVER_ROUTINE_H
#define FUNC_TABLE_SIZE 256

typedef struct SReceiveParam
{
    int clientSock;
    char* host;
    unsigned short port;
} SReceiveParam;

void* TcpReceiveRoutine(void* param);
void* UdpReceiveRoutine(void* param);

#endif