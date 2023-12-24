#pragma once

#define _GNU_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "../queue/task.h"
#include "../queue/queue.h"
#include "../log/log.h"

typedef struct tPool {
    pthread_t *threads;
    int nThreads;

    queueT *queue;
    pthread_mutex_t *qMutex;
    sem_t *sem;

    bool stopping;
    bool stopped;
} tPoolT;

tPoolT *tPoolNew(int nThreads);

void tPoolFree(tPoolT *tPool);

int tPoolStart(tPoolT *tPool);

int tPoolAddTask(tPoolT *tPool, taskT *task);

int tPoolStop(tPoolT *tPool);
