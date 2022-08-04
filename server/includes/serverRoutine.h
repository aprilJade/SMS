#ifndef SERVER_ROUTINE_H
#define SERVER_ROUTINE_H
#define FUNC_TABLE_SIZE 256

typedef struct SServRoutineParam
{
    int clientSock;
    int serverSock;
} SServRoutineParam;

enum eRoutine
{
    CPU_INFO = 'c',
    MEM_INFO = 'm',
    PROC_INFO = 'p',
    NET_INFO = 'n'
};

void initRoutineFuncTable(void **funcTable);
void* serverCpuInfoRoutine(void* param);
void* serverMemInfoRoutine(void* param);

#endif