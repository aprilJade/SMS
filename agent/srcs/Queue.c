#include "Queue.h"
#include <stdlib.h>

Queue* NewQueue()
{
    Queue* ret = (Queue*)malloc(sizeof(Queue));
    pthread_mutex_init(&ret->lock, NULL);
    ret->head = 0;
    ret->tail = 0;
    ret->count = 0;
    return ret;
}

int IsEmpty(Queue* queue)
{
    return 0 == queue->count;
}

int IsFull(Queue* queue)
{
    return queue->count == QUEUE_SIZE;
}

void* Pop(Queue* queue)
{
    void* ret;
    if (queue->count == 0)
        return NULL;
    ret = queue->data[queue->head++];
    queue->head %= QUEUE_SIZE;
    queue->count--;
    return ret;
}

int Push(void* data, Queue* queue)
{
    if (queue->count == QUEUE_SIZE)
    {
        return 1;    
    }
    queue->data[queue->tail] = data;
    queue->tail += 1;
    queue->tail %= QUEUE_SIZE;
    queue->count++;
    return 0;
}
