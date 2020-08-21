//
// Created by ingramani on 2/13/2019.
//

#ifndef _APP_CIRCULARQUEUE_H
#define _APP_CIRCULARQUEUE_H

#include <sys/types.h>

typedef struct QElementTag{

    unsigned char   *buf;
    int             size;

}qElement;


typedef struct cQueueTag{

    int     len;
    int     rPos;
    int     wPos;
    int     fPos;

    qElement        **elements;
    pthread_mutex_t qMutex;

}cQueue_t;

cQueue_t* createCirQueue(int maxSize);

int insertCirQueue(cQueue_t *cQueue, qElement *element);

int getCirQueue(cQueue_t *cQueue, qElement *element);

int isEmpty(cQueue_t *cQueue);

int isFull(cQueue_t *cQueue);

int flushCirQueue(cQueue_t *cQueue);

int destroyCirQueue(cQueue_t *cQueue);

#endif //_APP_CIRCULARQUEUE_H
