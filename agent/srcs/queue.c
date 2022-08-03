#include "queue.h"
#include <assert.h>
#include <stdlib.h>

static void qDeleteHead(Queue* queue, void (*del)(void *))
{
    assert(queue != NULL);
    Queue* tmp = queue->next;
    queue->next = tmp->next;
    del(tmp->pData);
    free(tmp);
}

void*   qGetHead(Queue* queue)
{
    assert(queue != NULL);
    if (queue->pData == NULL)
        return NULL;
    return (queue->next->pData);
}

void    qAddTail(Queue* queue, const void* data)
{
    assert(queue != NULL);
    while (queue->next)
        queue = queue->next;
    Queue* newNode = (Queue *)malloc(sizeof(Queue));
    newNode->pData = data;
    newNode->next = NULL;
    queue->next = newNode;
}

bool    qIsEmpty(Queue* queue)
{
    assert(queue != NULL);
    return (queue->pData == NULL);
}

size_t  qGetCount(Queue* queue)
{
    size_t ret = 0;
    assert(queue != NULL);
    while (queue->next)
    {
        ret++;
        queue = queue->next;
    }
    return ret;
}
