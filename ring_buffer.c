
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>
#include <android/log.h>
#include "ring_buffer.h"

//#define RINGBUFFER_DEBUG

static void debugHelper(ringBuffer_t *t, char *pData, int reqBytes){

    if(pData) {  //READ or WRITE
        //printf("Operation : %s \n", pData);
        __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Operation : %s \n", pData);

    }
#if 0
    //printf("The Ring Buffer Detail \n");
    printf("Front   :[%d] \n", t->front);
    printf("Rear    :[%d] \n", t->rear);
    printf("ReqBytes:[%d] \n", reqBytes);
    printf("FreeSize:[%d] \n", t->freeBufSize);
    printf("BufSize :[%d] \n", t->bufSize);
    printf("Avl.Size:[%d] \n", t->readableSize);
    printf("vReadPos:[%d] \n", t->vReadPos);
    printf("\n");
#endif
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Front   :[%d] \n", t->front);
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Rear    :[%d] \n", t->rear);
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "ReqBytes:[%d] \n", reqBytes);
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "FreeSize:[%d] \n", t->freeBufSize);
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "BufSize :[%d] \n", t->bufSize);
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Avl.Size:[%d] \n", t->readableSize);
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "vReadPos:[%d] \n", t->vReadPos);
}

int ringbuffer_init(int numOfElement, int size) {

    //TODO ??

    return 0;
}

ringBuffer_t*  create_ringBuffer(int size){

    ringBuffer_t    *p_rBuf = NULL;
    unsigned char   *p_tBuf = NULL;

    //Ring Buffer.
    p_rBuf = (ringBuffer_t*) malloc(sizeof (ringBuffer_t));
    if(p_rBuf == NULL) {
        return NULL; // Memory allocation Failure.
    }

    memset(p_rBuf, 0, sizeof(ringBuffer_t));

    //Allocate Byte Buffer.
    p_tBuf = (unsigned char*) malloc(sizeof(unsigned char) * size);
    if(p_tBuf == NULL) {
        free(p_rBuf);
        return NULL;
    }

    //Memset ??
    memset(p_rBuf,0, sizeof(ringBuffer_t));

    //Initialize RingBuffer.
    p_rBuf->buff = p_tBuf;
    p_rBuf->bufSize = size;
    p_rBuf->elementCount = size;
    p_rBuf->freeBufSize  = size;
    p_rBuf->front   = 0;
    p_rBuf->rear    = 0;
    p_rBuf->vReadPos = 0;
    p_rBuf->readableSize = 0;
    p_rBuf->processedBytes = 0;

    pthread_mutex_init(&p_rBuf->bufMutex, NULL);

    //Ring Buffer Instance
    return p_rBuf;

}

int ringbuffer_destroy(ringBuffer_t *p_ringBuffer){

    free(p_ringBuffer->buff);

    pthread_mutex_destroy(&p_ringBuffer->bufMutex);

    free(p_ringBuffer);

    return 0;
}

int ringbuffer_isEmpty(ringBuffer_t *p_ringBuffer) {

    return (p_ringBuffer->readableSize == 0 ? 1 : 0);
}

int ringbuffer_isFull(ringBuffer_t *p_ringBuffer) {

    return (p_ringBuffer->freeBufSize == 0 ? 1 : 0);
}

int ringbuffer_processedBytes(ringBuffer_t  *p_ringBuffer){

    return (p_ringBuffer->processedBytes);
}

int ringbuffer_enqueue(ringBuffer_t *p_ringBuffer, ringElement_t *element) {

    int             s32_size = 0;
    int             i = 0;
    int             index = 0;

    ringBuffer_t    *p_rBuf = NULL;

    if(p_ringBuffer == NULL || element == NULL) {
        return -2;
    }

    s32_size = element->size;

    p_rBuf =  p_ringBuffer;

    //Is there Space ?
    if(p_rBuf->freeBufSize == 0){
        return -5;  //Full
    }

    //enough space to copy
    if (p_rBuf->freeBufSize < s32_size) {
        return -6; // No Space to dump.
    }

    //Lock
    pthread_mutex_lock(&p_ringBuffer->bufMutex);
#ifdef RINGBUFFER_DEBUG
    debugHelper(p_rBuf, "WRITE", s32_size);
#endif
    //use %

#if 0
    //convert short to Float
    float *f32_ptr = (float*) p_rBuf->buff;
    short *s16_ptr = (short*) element->buf;

    index = p_rBuf->front % p_rBuf->bufSize;
    for(i = 0; i < s32_size; i++, index ++){
        f32_ptr[(i + p_rBuf->front) % p_rBuf->bufSize ] = (float)(s16_ptr[i] * (1.0f / SHRT_MAX)) ;
        p_rBuf->front += sizeof(float) ;
        p_rBuf->front %= p_rBuf->bufSize;
    }

    //successfully pushed.
    p_rBuf->freeBufSize -= s32_size * sizeof(float);
    p_rBuf->readableSize += s32_size * sizeof(float);

#else
    for(i = 0, index = 0; i < s32_size; i++) {  //Copy the buffer.

        index = p_rBuf->front % p_rBuf->bufSize;
        p_rBuf->buff[index] = element->buf[i];
        p_rBuf->front++;
    }

    //successfully pushed.
    p_rBuf->freeBufSize -= s32_size;
    p_rBuf->front = index + 1;
    p_rBuf->readableSize += s32_size;

#endif

#ifdef RINGBUFFER_DEBUG
    debugHelper(p_rBuf, "WRITE_DONE", s32_size);
#endif

    //Unlock
    pthread_mutex_unlock(&p_ringBuffer->bufMutex);

    return s32_size;
}

int ringbuffer_getProcessBuff(ringBuffer_t *p_ringBuffer, ringElement_t *element){


    return 0;
}

/* Get Buffer but don't move it */
int ringbuffer_getBuff(ringBuffer_t *p_ringBuffer, ringElement_t *element) {

    ringBuffer_t    *p_rBuf = NULL;
    int             s32_availSize = 0;

    if(p_ringBuffer == NULL || element == NULL) {
        return -2;
    }

    if(element->size <= 0){  //invalid request.
        return -3;
    }

    p_rBuf = p_ringBuffer;

    //Readable Size ?
    if(p_rBuf->readableSize == 0) {
        return -4; //Nothing to Read
    }

    pthread_mutex_lock(&p_ringBuffer->bufMutex);

    if(p_rBuf->vReadPos >= p_rBuf->front) {

        //AvailSize
        s32_availSize = p_rBuf->bufSize - p_rBuf->vReadPos;

    }else{
        s32_availSize = p_rBuf->front - p_rBuf->vReadPos;
    }

    if(element->size < s32_availSize) {
        s32_availSize = element->size;
    }
    else {
        element->size = s32_availSize;
    }

    element->buf = p_rBuf->buff + p_rBuf->vReadPos;

    //Reduce Readable Size
    p_rBuf->readableSize -= s32_availSize;

    //Update Virtual Read Position.
    p_rBuf->vReadPos += s32_availSize;
    p_rBuf->vReadPos %= p_rBuf->bufSize;

#ifdef RINGBUFFER_DEBUG
    debugHelper(p_rBuf, "READ", s32_availSize);
#endif

    pthread_mutex_unlock(&p_ringBuffer->bufMutex);

    //Don't update Rear and FreeSize here.

    return s32_availSize;
}

int ringbuffer_mvBuff(ringBuffer_t *p_ringBuffer, ringElement_t *element) {   //Put buffer.

    ringBuffer_t    *p_rBuf = NULL;
    int             s32_reqSize = 0;

    if(p_ringBuffer == NULL || element == NULL) {
        return -2;
    }

    s32_reqSize = element->size;

    p_rBuf = p_ringBuffer;

    //Make sure the correct buffer coming back ?
    if(element->buf != p_rBuf->buff + p_rBuf->rear){
        //Invalid Buffer and size.
        return -3;
    }

    pthread_mutex_lock(&p_ringBuffer->bufMutex);

    p_rBuf->rear += s32_reqSize;
    p_rBuf->rear %= p_rBuf->bufSize;

    p_rBuf->freeBufSize += s32_reqSize;

    //TOTAL processed.
    p_rBuf->processedBytes += s32_reqSize;

#ifdef RINGBUFFER_DEBUG
    debugHelper(p_rBuf, "READ_DONE", s32_reqSize);
#endif

    pthread_mutex_unlock(&p_ringBuffer->bufMutex);

    return 0;
}


int ringbuffer_dequeue(ringBuffer_t *p_ringBuffer, ringElement_t *element) {

    //Get Buf and Move Buf.
    return -1; //Unimplemented.
}

int ringbuffer_flush(ringBuffer_t *p_ringBuffer) {

    if(p_ringBuffer == NULL ) {
        return -2;
    }

    pthread_mutex_lock(&p_ringBuffer->bufMutex);

#ifdef RINGBUFFER_DEBUG
    debugHelper(p_ringBuffer, "Before Flush", 0);
#endif

    p_ringBuffer->front   = 0;
    p_ringBuffer->rear    = 0;
    p_ringBuffer->vReadPos = 0;
    p_ringBuffer->readableSize = 0;
    p_ringBuffer->processedBytes = 0;
    p_ringBuffer->freeBufSize = p_ringBuffer->bufSize;

#ifdef RINGBUFFER_DEBUG
    debugHelper(p_ringBuffer, "After Flush", 0);
#endif

    pthread_mutex_unlock(&p_ringBuffer->bufMutex);

    return 0;
}


int ringbuffer_deInit(void) {

    //TODO
    return 0;
}
