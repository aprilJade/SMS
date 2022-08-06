#ifndef QUEUE_H
#define QUEUE_H
#define QUEUE_SIZE 1024
#include <pthread.h>

typedef struct Queue
{
    void* data[QUEUE_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int count;
    pthread_mutex_t lock;
} Queue;

Queue* NewQueue();
int IsEmpty(Queue* queue);
int IsFull(Queue* queue);
void* Pop(Queue* queue);
int Push(void* data, Queue* queue);

#endif