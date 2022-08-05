#ifndef SERVER_ROUTINE_H
#define SERVER_ROUTINE_H
#define FUNC_TABLE_SIZE 256

typedef struct SServRoutineParam
{
    int clientSock;
    int serverSock;
} SServRoutineParam;

int ServCpuInfoRoutine(SServRoutineParam* param);
int ServMemInfoRoutine(SServRoutineParam* param);
int ServNetInfoRoutine(SServRoutineParam* param);
int ServProcInfoRoutine(SServRoutineParam* param);

#endif