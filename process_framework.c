//
// Created by ingramani on 1/29/2019.
//
#include <sys/types.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <memory.h>
#include <asm/errno.h>
#include "process_framework.h"
#include "ring_buffer.h"
#include "circularQueue.h"

#define REQ_BYTE_SIZE (2*1024)

#define PARALLEL_EXEC

typedef enum eNodeStateTag{

    PROCESS_NODE_WAITING_TO_INIT = 100,
    PROCESS_NODE_WAIT,
    PROCESS_NODE_WAITING,
    PROCESS_NODE_RUN,
    PROCESS_NODE_RUNNING,
    PROCESS_NODE_STOPPING,
    PROCESS_NODE_STOPPED,

}eNodeState_t;


typedef struct bufferChunks_Tag{

    unsigned char   *buf;
    int             bufSize;

}pBufChunks_t;


typedef struct pFrameworkTag{

    int                 numOfNode;
    struct pNodeTag      **node;   // Node List.

    void                *bufObj;

}pFramework_t;


typedef struct pNodeTag{

    int             id;
    int             initDone;  //Move into state.

    pNodeCore_t     *pCore;
    pthread_t       nodePid;
    pthread_mutex_t nodeMutex;
    pthread_cond_t  nodeCond;

    eNodeState_t    eState;   // ProcessState.

    cQueue_t        *processQueue;

    //Parent reference
    pFramework_t    *fwHandle;

    //Temporary fix.         //TODO: remove this idea.
    qElement        *tElement;

}pNode_t;



static pBufChunks_t getBufChunk(pNode_t *node){

    //Based on Node type
    pFramework_t    *fwHandle = NULL;
    ringElement_t   bufElement;
    pBufChunks_t    bufChunks;
    int             i = 0;
    int             retValue = 0;

    fwHandle = node->fwHandle;

    for(i = 0; i < fwHandle->numOfNode; i ++){

        if(node == fwHandle->node[i]){
            break;
        }
    }

    //First Node.
    if(i == 0){
        //Read from Buffer Directly.
        bufElement.size = REQ_BYTE_SIZE;
        retValue = ringbuffer_getBuff((ringBuffer_t*)  fwHandle->bufObj, &bufElement);
        if(retValue < 0){
           //TODO: Wait ?
           usleep(5);
            //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "No input data from RingBuf and retValue  %d \n",
            //                    bufChunks.bufSize);
        }
        else{
            usleep(5); //No Sleep ?
            //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Successful Dequeue from RingBuf and Size  %d \n",
            //                    retValue);
        }
        bufChunks.buf = bufElement.buf;
        bufChunks.bufSize = retValue;

    }else{

        qElement element;

        //get From Previous Node.
        if(! isEmpty(fwHandle->node[i-1]->processQueue) ) {

            getCirQueue(fwHandle->node[i-1]->processQueue, &element);

            bufChunks.buf = element.buf;
            bufChunks.bufSize = element.size;

            //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Successful Dequeue at  %s and size %d \n",
            //                    fwHandle->node[i-1]->pCore->name, bufChunks.bufSize);
        }
        else {
            //No data.
            bufChunks.bufSize = 0;
            bufChunks.buf     = NULL;

            //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Failed to Dequeue at  %s \n",
            //                    fwHandle->node[i - 1]->pCore->name);
        }
    }

    return bufChunks ; // copy
#if 0
    if(bufChunks.bufSize > 0) {

        qElement element;

        element.size = bufChunks.bufSize;
        element.buf  = bufChunks.buf;

        __android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Enqueue at  %s \n", node->pCore->name);

        //Preserve
        insertCirQueue(node->processQueue, &element);
    }

    return bufChunks;  //copy
#endif
}

static int putBufChunk(pNode_t *node, pBufChunks_t *bufChunks){

    int i = 0;

    ringElement_t   element;
    qElement        bufElement;
    pFramework_t    *fwHandle = NULL;

    fwHandle =  node->fwHandle;

    //Update Nodes' Chunk
    bufElement.size = bufChunks->bufSize;
    bufElement.buf  = bufChunks->buf;

    //Preserve Processed data.
    insertCirQueue(node->processQueue, &bufElement);

    for(i = 0; i < fwHandle->numOfNode; i ++){

        if(node == fwHandle->node[i]){
            break;
        }
    }

    if( i == (fwHandle->numOfNode - 1)){  //If it is Final Node data Move back to ringBuffer.

        getCirQueue(node->processQueue, &bufElement);  //TODO: Rename and unify "Buf Element".
        element.buf = bufElement.buf;
        element.size = bufElement.size;

        //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Successful Dequeue at  %s \n", node->pCore->name);
        ringbuffer_mvBuff((ringBuffer_t*)fwHandle->bufObj, &element);
    }

    return 0;  //Success.
}

static int emitSignal(pNode_t *node){

    int i = 0;

    pFramework_t    *fwHandle = NULL;
    pNode_t         *nextNode = NULL;

    fwHandle = node->fwHandle;

    for(i = 0; i < fwHandle->numOfNode; i ++){

        if(node == fwHandle->node[i]){
            break;
        }
    }
#ifdef PARALLEL_EXEC
    //End of node list.
    if( i == (fwHandle->numOfNode - 1)){

        return 0;  //TODO: rename return code. (pcm Buff)
    }
#endif

    nextNode = fwHandle->node[(i + 1) % fwHandle->numOfNode];


    //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Make sure name %s State %d\n",
    //                    nextNode->pCore->name, nextNode->eState);

    //Check Processing node settled.
    do{
        usleep(50);

        if(nextNode->eState == PROCESS_NODE_STOPPING ||
           nextNode->eState == PROCESS_NODE_STOPPED ){

            return 0;
        }
    }while(nextNode->eState != PROCESS_NODE_WAITING);

    //Lock
    pthread_mutex_lock(&nextNode->nodeMutex);

    //Set Runnable State
    nextNode->eState = PROCESS_NODE_RUN;

    //Awake it up.
    pthread_cond_signal(&nextNode->nodeCond);

    //Unlock, awoke process will proceed.
    pthread_mutex_unlock(&nextNode->nodeMutex);

    return 0;
}

static void updateNodeState(pNode_t *node){

    int             i = 0;
    pFramework_t    *fwHandle = NULL;

    fwHandle = node->fwHandle;

    for(i = 0; i < fwHandle->numOfNode; i ++){

        if(node == fwHandle->node[i]){
            break;
        }
    }

    if ( i != 0) {    //other then the first node.

        //Set to Waiting.
        node->eState = PROCESS_NODE_WAITING;
    }

    return;
}

/* Thread Process */
static void* nodeProcessing(void *pArgs){

    pBufChunks_t        bufChunk;
    int                 s32_retValue;

    if(pArgs == NULL){
        pthread_exit(NULL);
    }

    pNode_t *node = (pNode_t*) pArgs;


    do{
        node->eState = PROCESS_NODE_WAITING;
        usleep(1);
    }while(!node->initDone);

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Thread Initialized : %d", pthread_self());

    do {

        switch (node->eState) {

            case PROCESS_NODE_WAITING_TO_INIT:
                 node->eState = PROCESS_NODE_WAIT;
                 break;       

            case PROCESS_NODE_WAITING:
            case PROCESS_NODE_WAIT   :

                pthread_mutex_lock(&node->nodeMutex);

                do {  
                    node->eState = PROCESS_NODE_WAITING;
                    pthread_cond_wait(&node->nodeCond, &node->nodeMutex);
                } while (node->eState == PROCESS_NODE_WAITING);

                pthread_mutex_unlock(&node->nodeMutex);
                break;

            case PROCESS_NODE_RUN:

                 node->eState = PROCESS_NODE_RUNNING;
                 break;

            case PROCESS_NODE_RUNNING:

                //Is outQueue full ?
                if(isFull(node->processQueue)){
                    usleep(100);
                    //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", " Queue FULL Processing %s", node->pCore->name);
#if 0
                    emitSignal(node);
#endif
                    continue;
                }

                //Check, is there any temporary element not pushed ?
                if(node->tElement->size > 0){

                    bufChunk.bufSize = node->tElement->size;
                    bufChunk.buf     = node->tElement->buf;
                }
                else {
                    //Get BuffChunk.
                    bufChunk = getBufChunk(node);
                    if (bufChunk.bufSize <= 0) {
                        usleep(10);
                        //Keep RUN.
                        // __android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue",
                        //                    "Invalid buffer for Processing %s", node->pCore->name);
                        usleep(100);
                        continue;
                    }
                }

                //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "Processing Id %s", node->pCore->name);
                __android_log_print(ANDROID_LOG_DEBUG, "", "" );

                //CoreProcess.
                do {

                   s32_retValue = node->pCore->fp_coreProcess((float *) bufChunk.buf, bufChunk.bufSize);
                   if(s32_retValue >= 0){
                       node->tElement->buf = NULL;
                       node->tElement->size = 0;

                       //Release Buffer
                       putBufChunk(node, &bufChunk);
                       break;
                   }
                   else if(s32_retValue < 0){

                       //Unsuccessful.. Keep in Temp..
                       node->tElement->buf = bufChunk.buf;
                       node->tElement->size = bufChunk.bufSize;

                       //__android_log_print(ANDROID_LOG_DEBUG, "ProcessQueue", "UnProcessed data on %s", node->pCore->name);
                       __android_log_print(ANDROID_LOG_DEBUG, "", "" );

                       usleep(100);
                       continue;
                   }

                }while(node->eState == PROCESS_NODE_RUNNING);

#ifdef PARALLEL_EXEC

#if 0           //Don't wake up and Don't change state.

                if(node->eState == PROCESS_NODE_RUNNING) {
                    //Signal for next Node.
                    emitSignal(node);
                }

                //Update eState.
                updateNodeState(node);
#endif
#else
                //Move to State for
                node->eState = PROCESS_NODE_WAITING;
#endif
                break;

            case PROCESS_NODE_STOPPING:
                node->eState = PROCESS_NODE_STOPPED;
                break;

            case PROCESS_NODE_STOPPED:
                break;

            default:
                //Unhandled here.
                break;
        }

        usleep(100);
    }while(node->eState!= PROCESS_NODE_STOPPED);

    pthread_exit(NULL);
}


static pNode_t* createNode(pFramework_t *fwHandle, pNodeCore_t *nodeCore){

    pNode_t         *tNode = NULL;
    int             retValue = 0;

    tNode = malloc(sizeof(pNode_t));
    if(tNode == NULL) {
        return tNode;
    }

    tNode->pCore = malloc (sizeof(pNodeCore_t));
    if(tNode->pCore == NULL) {
        free(tNode);
        return NULL;
    }

    //Copy Core.
    tNode->pCore->id             = nodeCore->id;
    tNode->pCore->fp_coreProcess = nodeCore->fp_coreProcess;
    tNode->pCore->name           = strdup(nodeCore->name);

    tNode->processQueue          = createCirQueue(32);

    tNode->tElement              = malloc(sizeof(qElement));
    tNode->tElement->buf = NULL;
    tNode->tElement->size = 0;

    pthread_mutex_init(&tNode->nodeMutex, NULL);
    pthread_cond_init(&tNode->nodeCond, NULL);

    tNode->initDone  = 0;
    tNode->eState    = PROCESS_NODE_WAITING_TO_INIT;
    tNode->fwHandle  = fwHandle;

    retValue = pthread_create(&tNode->nodePid, NULL, nodeProcessing, (void*)tNode);
    if(retValue != 0){
        free(tNode);
        return NULL;
    }

    //Set Thread Name.
    pthread_setname_np(tNode->nodePid, nodeCore->name);

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Node Created %s ", nodeCore->name);

    return tNode;
}

static int createAndInitNode(pFramework_t *fwHandle, int index, pNodeCore_t *nodeCore){

    fwHandle->node[index] = createNode(fwHandle, nodeCore);

    return 0; //Success.
}


static int startNode(pNode_t *node){

    if(node->eState == PROCESS_NODE_RUNNING){

        //Already running.
        return 0;
    }

    do{
        usleep(1);
    }while(node->eState != PROCESS_NODE_WAITING);

    //Start Node.
    pthread_mutex_lock(&node->nodeMutex);

    node->eState = PROCESS_NODE_RUN;

    pthread_cond_signal(&node->nodeCond);

    pthread_mutex_unlock(&node->nodeMutex);

    return  0;
}

static int suspendNode(pNode_t *node){

    if(node->eState == PROCESS_NODE_STOPPED){

        //Already Stopped.
        return -1;
    }

__android_log_print(ANDROID_LOG_DEBUG,"ProcessState", "Node State %d", node->eState);
    if(node->eState != PROCESS_NODE_WAITING) {

        //Bring to Idle.
        node->eState = PROCESS_NODE_WAITING_TO_INIT;

        do{
            usleep(100);
            __android_log_print(ANDROID_LOG_DEBUG,"ProcessState", "Node State %d name %s", node->eState, node->pCore->name);
        }while(node->eState != PROCESS_NODE_WAITING);

    }

__android_log_print(ANDROID_LOG_DEBUG,"ProcessState", "Node State %d", node->eState);
    return 0;
}

static int stopNode(pNode_t *node){

    int retValue = 0;

    if(node->eState == PROCESS_NODE_STOPPED){

        //Already Stopped.
        return 0;
    }

    if(node->eState != PROCESS_NODE_WAITING) {

        //Bring to Idle.
        node->eState = PROCESS_NODE_WAITING_TO_INIT;
    
        do{
           usleep(10);
        }while(node->eState != PROCESS_NODE_WAITING);
    }

    //Awake up.
    pthread_mutex_lock(&node->nodeMutex);

    node->eState = PROCESS_NODE_STOPPING;

    retValue = pthread_cond_signal(&node->nodeCond);

    pthread_mutex_unlock(&node->nodeMutex);

    //Make sure STOPPED
    do{
        usleep(10);
    }while(node->eState != PROCESS_NODE_STOPPED);

    return  0;
}

/************************** Public APIs*****************************************/

int init_process_framework(void){

    //TODO ??
    return 0;
}



long create_process_framework(void *bufObj, processFwArgs_t *pNodeArgs){

    pFramework_t    *fwHandle = NULL;

    if(bufObj == NULL || pNodeArgs == NULL){
        return -1;
    }

    fwHandle = malloc(sizeof(pFramework_t));
    if(fwHandle == NULL ){
        return -2;
    }

    fwHandle->numOfNode = pNodeArgs->numOfNode;
    fwHandle->node      = (pNode_t**) malloc(sizeof(pNode_t*) * fwHandle->numOfNode);
    fwHandle->bufObj    = bufObj;

    for(int i = 0; i < pNodeArgs->numOfNode; i++) {

        createAndInitNode(fwHandle, i, &pNodeArgs->nodeCore[i]);
    }

    //Make sure threads are created and awaiting..
    do{
        usleep(5); //Let thread to Settle.

        for(int i = 0; i < pNodeArgs->numOfNode; i++){

            do{
                  usleep(1);

            }while(!(fwHandle->node[i]->eState == PROCESS_NODE_WAITING));
            fwHandle->node[i]->initDone = 1;  
        }

    }while(0);


    //Threads are Initialized and awaiting to start.

    //Success.
    return (long) fwHandle;
}

long startProcessFramework(long handle){

    //Trigger Node 1.
    pFramework_t *fwHandle = NULL;

    if(handle <= 0){
        return -2;
    }

    fwHandle = (pFramework_t*) handle;

    //startNode(fwHandle->node[0]);

//#ifdef NO_SIGNAL
    for(int i = 0; i < fwHandle->numOfNode; i ++) {
        __android_log_print(ANDROID_LOG_DEBUG, "ProcessState", "Node State ID %d", i);
        startNode(fwHandle->node[i]);
    }
//#endif
    return 0;
}

long suspendProcessFramework(long handle, int flush){

    int          i = 0;
    pFramework_t *fwHandle = NULL;

    if(handle <= 0){
        return -2;
    }

    fwHandle = (pFramework_t*) handle;

    __android_log_print(ANDROID_LOG_DEBUG,"ProcessState", "Firmwork Bring to Pause Start");
    for(i = 0; i < fwHandle->numOfNode; i ++) {
        __android_log_print(ANDROID_LOG_DEBUG, "ProcessState", "Node State ID %d", i);
        suspendNode(fwHandle->node[i]);
    }
    __android_log_print(ANDROID_LOG_DEBUG,"ProcessState", "Firmwork Bring to Pause End");

    if(flush) {
        //Flush Node.
        for (i = 0; i < fwHandle->numOfNode; i++) {
            //Flush elements.
            flushCirQueue(fwHandle->node[i]->processQueue);

            fwHandle->node[i]->tElement->buf = NULL;
            fwHandle->node[i]->tElement->size = 0;
        }

        __android_log_print(ANDROID_LOG_DEBUG,"ProcessState", "Firmwork Flushed buffer nodes");
    }

    return  0;
}

long stopProcessFramework(long handle){

    int          i = 0;
    pFramework_t *fwHandle = NULL;

    if(handle <= 0){
        return -2;
    }

    fwHandle = (pFramework_t*) handle;

    for(i = 0; i < fwHandle->numOfNode; i ++) {
        stopNode(fwHandle->node[i]);
        pthread_join(fwHandle->node[i]->nodePid, NULL);
    }
    return  0;
}

long updateProcessInputs(long handle){

    //TODO
    return 0;
}

long terminateProcessFramework(long handle) {  // Destroy

    pFramework_t *fwHandle = NULL;

    if(handle <= 0){
        return -2;
    }

    fwHandle = (pFramework_t*) handle;

    stopProcessFramework(handle);

    for(int i = 0; i < fwHandle->numOfNode; i ++){

        destroyCirQueue(fwHandle->node[i]->processQueue);
        free(fwHandle->node[i]->tElement);
        free(fwHandle->node[i]->pCore->name);
        free(fwHandle->node[i]->pCore);
        free(fwHandle->node[i]);
    }

    free(fwHandle->node);

    free(fwHandle);

    return 0;
}

long flushRestart(long handle){

    //Suspend.
    suspendProcessFramework(handle, 1);

    //Restart
    startProcessFramework(handle);

    return 0;
}

int deInit_process_framework(void){

    //TODO:

    return 0;
}

