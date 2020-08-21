//
// Created by ingramani on 2/13/2019.
//

#include <stdint.h>
#include <malloc.h>
#include <pthread.h>
#include "circularQueue.h"

#define  MAX_QUEUE_SIZE  64

cQueue_t* createCirQueue(int size){

    cQueue_t *queue = NULL;

    if(size > MAX_QUEUE_SIZE || size <= 0){
        size = MAX_QUEUE_SIZE;
    }

    queue = malloc(sizeof(cQueue_t));
    if(queue == NULL){
        return -2;
    }

    //array of Pointers.
    queue->elements = (qElement**) malloc(sizeof(qElement*) * size);
    if(queue->elements == NULL){
        free(queue);
        return -2;
    }

    for(int i = 0; i < size; i++){
        queue->elements[i] = (cQueue_t*) malloc(sizeof(qElement));  //TODO: NULL Check.
        queue->elements[i]->size = 0;
        queue->elements[i]->buf  = NULL;
    }

    queue->len  = size;
    queue->rPos = 0;
    queue->wPos = 0;
    queue->fPos = size; //FreePos

    pthread_mutex_init(&queue->qMutex, NULL);
    return queue;
}

int flushCirQueue(cQueue_t *cQueue){

    pthread_mutex_lock(&cQueue->qMutex);

    cQueue->rPos = 0;
    cQueue->wPos = 0;
    cQueue->fPos = cQueue->len;  //FreePos

    pthread_mutex_unlock(&cQueue->qMutex);

    return 0;
}

int insertCirQueue(cQueue_t *cQueue, qElement *element){


    if(cQueue == NULL || element == NULL){
        return -1;
    }

    if(cQueue->fPos == 0 ){
        return -2; //No Space
    }

    pthread_mutex_lock(&cQueue->qMutex);

    //Copy data points
    cQueue->elements[cQueue->wPos]->buf = element->buf;
    cQueue->elements[cQueue->wPos]->size = element->size;

    cQueue->wPos++;
    cQueue->fPos--;

    cQueue->wPos %= cQueue->len;

    pthread_mutex_unlock(&cQueue->qMutex);

    return 0;
}

int getCirQueue(cQueue_t *cQueue, qElement *element){

    if(cQueue == NULL || element == NULL){
        return -1;
    }

    if(cQueue->fPos == cQueue->len){  //Nothing can Read.
        return -2;
    }

    pthread_mutex_lock(&cQueue->qMutex);

    //Copy data points
    element->buf = cQueue->elements[cQueue->rPos] ->buf;
    element->size = cQueue->elements[cQueue->rPos]->size ;

    cQueue->rPos++;
    cQueue->fPos++;

    cQueue->rPos %= cQueue->len;

    pthread_mutex_unlock(&cQueue->qMutex);

    return 0;
}

int flushQueue(cQueue_t *cQueue){

    if(cQueue == NULL){
        return -1;
    }

    pthread_mutex_lock(&cQueue->qMutex);

    cQueue->fPos = cQueue->len;
    cQueue->rPos = 0;
    cQueue->wPos = 0;

    pthread_mutex_unlock(&cQueue->qMutex);

    return 0;
}

int isEmpty(cQueue_t *cQueue){

    if(cQueue == NULL){
        return -1;
    }

    return (cQueue->fPos == cQueue->len) ? 1 : 0;
}

int isFull(cQueue_t *cQueue){

    if(cQueue == NULL){
        return -1;
    }

    return (cQueue->fPos == 0) ? 1 : 0;
}


int destroyCirQueue(cQueue_t *cQueue){

    if(cQueue == NULL){
        return -1;
    }

    //Clear memory.
    pthread_mutex_lock(&cQueue->qMutex);

    cQueue->fPos = 0;

    for(int i =0; i < cQueue->len; i ++){
        free(cQueue->elements[i]);
    }

    free(cQueue->elements);

    pthread_mutex_unlock(&cQueue->qMutex);

    //Destroy
    pthread_mutex_destroy(&cQueue->qMutex);

    free(cQueue);

    return 0;
}
