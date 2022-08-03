#ifndef QUEUE_H
#define QUEUE_H
#include <stddef.h>
#include <stdbool.h>

// Just FIFO Queue
typedef struct Queue
{
    void    *pData;
    Queue   *next;    
} Queue;

void*   qGetHead(Queue* queue);
void    qAddTail(Queue* queue, const void* data);
bool    qIsEmpty(Queue* queue);
size_t  qGetCount(Queue* queue);

#endif