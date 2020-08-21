//
// Created by ingramani on 2/6/2019.
//

#ifndef _LISTQUEUE_H
#define _LISTQUEUE_H


/* a link in the queue, holds the info and point to the next Node*/
typedef struct {
    int             info;
    unsigned char   *ptr;
    int             size;
} DATA;

typedef struct Node_t {
    DATA data;
    struct Node_t *prev;
} NODE;

/* the HEAD of the Queue, hold the amount of node's that are in the queue*/
typedef struct Queue {
    NODE *head;
    NODE *tail;
    int size;
    int limit;

    pthread_mutex_t qMutex;
} Queue;

Queue   *ConstructQueue(int limit);
void    DestructQueue(Queue *queue);
int     Enqueue(Queue *pQueue, NODE *item);
NODE    *Dequeue(Queue *pQueue);
int     isEmpty(Queue* pQueue);
int     isFull(Queue* pQueue);
void    flushQueue(Queue *pQueue);


#endif //_LISTQUEUE_H
