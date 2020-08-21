//
// Created by ingramani on 2/4/2019.
//

#include <android/log.h>
#include <zconf.h>
#include <xaf_interface.h>
#include <sys/time.h>
#include "testFunctions.h"


void dummy_functions_1(unsigned char *buf, const int size){

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Core Functions getting invoked 1");
    usleep(100);
    return;
}

void dummy_functions_2(unsigned char *buf, const int size){

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Core Functions getting invoked 2");
    usleep(123);
    return;
}

void dummy_functions_3(unsigned char *buf, const int size){

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Core Functions getting invoked 3");
    usleep(344);

    return;
}

void dummy_functions_4(unsigned char *buf, const int size){

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Core Functions getting invoked 4");
    usleep(411);

    return;
}

void dummy_functions_5(unsigned char *buf, const int size){

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Core Functions getting invoked ");
    return;
}


void gain_function(unsigned char *buf, const int size){

    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Core Functions getting invoked 1");
    //usleep(100);
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    xaf_process_buffer(0, 4, buf, size , NULL, 0, 0);

    gettimeofday(&t2, NULL);

    //TODO Rand ?
    __android_log_print(ANDROID_LOG_DEBUG, "ProcessFramework", "Time for Gain Processing %ld ",
                        ((t2.tv_sec * 1000000 + t2.tv_usec)
                         - (t1.tv_sec * 1000000 + t1.tv_usec)));
    return;
}
