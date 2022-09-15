#ifndef QUEUE_H
#define QUEUE_H
#include <pthread.h>

typedef struct Queue
{
    void* data;
    pthread_mutex_t lock;
    struct Queue* next;
} Queue;

Queue* NewQueue();
int IsEmpty(Queue* queue);
void* Pop(Queue* queue);
int Push(void* data, Queue* queue);

#endif