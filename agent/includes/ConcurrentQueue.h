#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_SIZE 3
#include <pthread.h>

typedef struct ConcurrentQueue
{
    void* data[CONCURRENT_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
} ConcurrentQueue;

ConcurrentQueue* NewQueue();
int CQIsEmpty(ConcurrentQueue* queue);
void* CQPop(ConcurrentQueue* queue);
int CQPush(void* data, ConcurrentQueue* queue);

#endif