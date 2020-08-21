
#include <malloc.h>
#include <pthread.h>
#include "listQueue.h"

//code copied from https://gist.github.com/ArnonEilat/4471278

#define TRUE  1
#define FALSE	0


Queue *ConstructQueue(int limit) {

    Queue *queue = (Queue*) malloc(sizeof (Queue));
    if (queue == NULL) {
        return NULL;
    }
    if (limit <= 0) {
        limit = 65535;
    }
    queue->limit = limit;
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;

    pthread_mutex_init(&queue->qMutex, NULL);

    return queue;
}

void FlushQueue(Queue *queue){

    //DeQueue all the items
    NODE *pN;
    while (!isEmpty(queue)) {
        pN = Dequeue(queue);
        free(pN);
    }

    return;
}

void DestructQueue(Queue *queue) {
    NODE *pN;
    while (!isEmpty(queue)) {
        pN = Dequeue(queue);
        free(pN);
    }

    pthread_mutex_destroy(&queue->qMutex);

    free(queue);
}

int Enqueue(Queue *pQueue, NODE *item) {
    /* Bad parameter */
    if ((pQueue == NULL) || (item == NULL)) {
        return FALSE;
    }
    // if(pQueue->limit != 0)
    if (pQueue->size >= pQueue->limit) {
        return FALSE;
    }

    pthread_mutex_lock(&pQueue->qMutex);
    /*the queue is empty*/
    item->prev = NULL;
    if (pQueue->size == 0) {
        pQueue->head = item;
        pQueue->tail = item;

    } else {
        /*adding item to the end of the queue*/
        pQueue->tail->prev = item;
        pQueue->tail = item;
    }
    pQueue->size++;


    pthread_mutex_unlock(&pQueue->qMutex);
    return TRUE;
}

int isFull(Queue *pQueue){

    if(pQueue == NULL){
        return TRUE;
    }

    if(pQueue->size == pQueue->limit){
        return TRUE;

    }else{
        //Have Space.
        return FALSE;
    }
}

NODE * Dequeue(Queue *pQueue) {
    /*the queue is empty or bad param*/
    NODE *item;
    if (isEmpty(pQueue))
        return NULL;


    pthread_mutex_lock(&pQueue->qMutex);
    item = pQueue->head;
    pQueue->head = (pQueue->head)->prev;
    pQueue->size--;


    pthread_mutex_unlock(&pQueue->qMutex);
    return item;
}

int isEmpty(Queue* pQueue) {
    if (pQueue == NULL) {
        return FALSE;
    }
    if (pQueue->size == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

