/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2021
 *********************************************************/

#include "my_logger.h"

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

typedef struct MyLogger_ {
    pthread_mutex_t lock;
    FILE* out;
    const char* hostname;
} MyLogger;

MyLogger* new_my_logger(FILE* out, const char* hostname) {
    MyLogger* l = (MyLogger*)malloc(sizeof(MyLogger));

    l->out = out;
    l->hostname = hostname;

    pthread_mutexattr_t atr;
	pthread_mutexattr_init(&atr);
	pthread_mutex_init(&l->lock, &atr);

    return l;
}

void my_logger_write(MyLogger* logger, char* proto, char* event, char* desc) {
    assert(logger);

    char buffer[26];
	struct tm* tm_info;
    struct timespec tp;

    pthread_mutex_lock(&logger->lock);

    // Get Time
    clock_gettime(CLOCK_REALTIME, &tp);
    tm_info = localtime(&tp.tv_sec);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);

	/*int res = */fprintf(logger->out, "%s> [%s %0*ld] [%s] [%s] :: %s\n", logger->hostname, buffer, 9, tp.tv_nsec, proto, event, desc);

    //if(res < 0) {
    //    printf("Error writing to logger->out!\n");
    //}

	pthread_mutex_unlock(&logger->lock);
}

void my_logger_flush(MyLogger* logger) {
    assert(logger);
    pthread_mutex_lock(&logger->lock);
    fflush(logger->out);
    pthread_mutex_unlock(&logger->lock);
}


void my_logger_close(MyLogger* logger) {
    assert(logger);
    pthread_mutex_lock(&logger->lock);
    //fflush(logger->out);
    fclose(logger->out);
    pthread_mutex_unlock(&logger->lock);
}
