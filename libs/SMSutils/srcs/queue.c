#include "queue.h"
#include <stdlib.h>

Queue* NewQueue()
{
    Queue* ret = (Queue*)malloc(sizeof(Queue));
    if (ret == NULL)
        return NULL;
    pthread_mutex_init(&ret->lock, NULL);
    ret->data = NULL;
    ret->next = NULL;
    return ret;
}

int IsEmpty(Queue* queue)
{
    return queue->next == NULL;
}

void* Pop(Queue* queue)
{
    if (IsEmpty(queue))
        return NULL;

    Queue* tmp = queue->next;
    void* ret = tmp->data;
    queue->next = tmp->next;
    free(tmp);

    return ret;
}

int Push(void* data, Queue* queue)
{
    Queue* tmp = (Queue*)malloc(sizeof(Queue));
    if (tmp == NULL)
        return 1;

    tmp->data = data;
    tmp->next = NULL;
    while (queue->next)
        queue = queue->next;
    queue->next = tmp;
    return 0;
}
