//
// Created by ingramani on 1/29/2019.
//

#ifndef _APP_PROCESS_FRAMEWORK_H
#define _APP_PROCESS_FRAMEWORK_H


#include <sys/types.h>

typedef int (*nodeProcess)(float *inBuf, int bytes);


typedef struct pNodeCoreTag{

    int             id;
    nodeProcess     fp_coreProcess;
    char            *name;

}pNodeCore_t;


typedef struct frameworkProcessArg_Tag{

    pNodeCore_t     *nodeCore;
    int             numOfNode;

    //OutputCore.
    void*           outCore;

}processFwArgs_t;


int init_process_framework(void);

long create_process_framework(void *bufObj, processFwArgs_t *pNodeArgs);

long startProcessFramework(long handle);

long stopProcessFramework(long handle);

long suspendProcessFramework(long handle, int flush);

long updateProcessInputs(long handle);

long terminateProcessFramework(long handle); // Destroy

int deInit_process_framework(void);



#endif //APP_PROCESS_FRAMEWORK_H
