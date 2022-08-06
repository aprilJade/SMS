#include "ConcurrentQueue.h"
#include <stdlib.h>

ConcurrentQueue* NewQueue()
{
    ConcurrentQueue* ret = (ConcurrentQueue*)malloc(sizeof(ConcurrentQueue));
    pthread_mutex_init(&ret->lock, NULL);
    ret->head = 0;
    ret->tail = 0;
    ret->count = 0;
    return ret;
}

int CQIsEmpty(ConcurrentQueue* queue)
{
    int ret;
    pthread_mutex_lock(&queue->lock);
    ret = 0 == queue->count;
    pthread_mutex_unlock(&queue->lock);
    return ret;
}

void* CQPop(ConcurrentQueue* queue)
{
    void* ret;
    pthread_mutex_lock(&queue->lock);
    if (queue->count == 0)
    {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }
    ret = queue->data[queue->head++];
    queue->head %= CONCURRENT_QUEUE_SIZE;
    queue->count--;
    pthread_mutex_unlock(&queue->lock);
    return ret;
}

int CQPush(void* data, ConcurrentQueue* queue)
{
    int tmp;
    pthread_mutex_lock(&queue->lock);
    tmp = (queue->tail + 1) % CONCURRENT_QUEUE_SIZE;
    if (queue->count == CONCURRENT_QUEUE_SIZE)
    {
        pthread_mutex_unlock(&queue->lock);
        return 1;    
    }
    queue->data[queue->tail] = data;
    queue->tail = tmp;
    queue->count++;
    pthread_mutex_unlock(&queue->lock);
    return 0;
}
