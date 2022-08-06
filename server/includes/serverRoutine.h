#ifndef SERVER_ROUTINE_H
#define SERVER_ROUTINE_H
#define FUNC_TABLE_SIZE 256

typedef struct SServRoutineParam
{
    int clientSock;
    int serverSock;
    int logFd;
} SServRoutineParam;

void* ServCpuInfoRoutine(void* param);
void* ServMemInfoRoutine(void* param);
void* ServNetInfoRoutine(void* param);
void* ServProcInfoRoutine(void* param);

#endif