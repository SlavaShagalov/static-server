#pragma once

#define _GNU_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "../queue/task.h"
#include "../queue/queue.h"
#include "../log/log.h"

typedef struct tpool {
    pthread_t *th;
    int t_sz;

    queue_t *queue;
    pthread_mutex_t *q_mutex;
    sem_t *sem;

    bool stop;
    bool stopped;
} tpool_t;

tpool_t *new_tpool_t(int thread_num);

void free_tpool_t(tpool_t *pool_ptr);

int run_tpool_t(tpool_t *pool);

int add_task(tpool_t *pool, task_t *task);

int stop(tpool_t *pool);
