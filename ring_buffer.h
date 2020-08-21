
#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#include <sys/types.h>

typedef struct ringBuffer_T{

    unsigned char   *buff;           //Buffer.
    //float           *buff;
    int             elementCount;    //Reserved
    int             front;           //Write Position
    int             rear;            //Read Position
    int             bufSize;         //Size of RingBuffer
    int             freeBufSize;     //Bytes can write.

    int             readableSize;    //Bytes available for Reading.
    int             vReadPos;        //Virtual Reading Position

    int             processedBytes;

    unsigned char   *processBuf;
    int             processBuffSize;

    //Protection.
    pthread_mutex_t    bufMutex;     //Guard.

}ringBuffer_t;


typedef struct ringElement_T {

    unsigned char   *buf;
    //float           *buf;
    int             size;

}ringElement_t;


int ringbuffer_init(int numOfElement, int maxSize);

ringBuffer_t*  create_ringBuffer(int maxSize);

int ringbuffer_destroy(ringBuffer_t *p_buf);

int ringbuffer_enqueue(ringBuffer_t *p_buf, ringElement_t *element);

int ringbuffer_isFull(ringBuffer_t *p_ringBuffer);

int ringbuffer_isEmpty(ringBuffer_t *p_ringBuffer);

int ringbuffer_processedBytes(ringBuffer_t *p_ringBuffer);

int ringbuffer_getBuff(ringBuffer_t *p_ringBuffer, ringElement_t *element);

int ringbuffer_mvBuff(ringBuffer_t *p_ringBuffer, ringElement_t *element);

int ringbuffer_dequeue(ringBuffer_t *p_buf, ringElement_t *element);

int ringbuffer_flush(ringBuffer_t *p_ringBuffer);

int ringbuffer_deInit(void);

#endif //_RING_BUFFER_H
