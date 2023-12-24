#include "t_pool.h"

typedef struct routine_args {
    tPoolT *pool;
    int num;
} routine_args_t;

void *routine(void *routine_args);

routine_args_t *new_args(tPoolT *pool, int num);

tPoolT *tPoolNew(int nThreads) {
    tPoolT *pool = calloc(1, sizeof(tPoolT));
    if (pool == NULL) {
        logFatal(ERR_FSTR, "ThreadPool alloc failed", strerror(errno));
        return NULL;
    }

    pool->queue = queueNew();
    if (pool->queue == NULL) {
        logFatal(ERR_FSTR, "queue alloc failed", strerror(errno));
        free(pool);
        return NULL;
    }
    pool->queue->len = 0;

    pool->threads = calloc(nThreads, sizeof(pthread_t));
    if (pool->threads == NULL) {
        logFatal(ERR_FSTR, "pthreads alloc failed", strerror(errno));
        queueFree(pool->queue);
        free(pool);
        return NULL;
    }
    pool->nThreads = nThreads;

    pool->qMutex = calloc(1, sizeof(pthread_mutex_t));
    if (pool->qMutex == NULL) {
        logFatal(ERR_FSTR, "mutex alloc failed", strerror(errno));
        queueFree(pool->queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    pool->sem = calloc(1, sizeof(sem_t));
    if (pool->sem == NULL) {
        logFatal(ERR_FSTR, "cond var alloc failed", strerror(errno));
        queueFree(pool->queue);
        free(pool->threads);
        free(pool->qMutex);
        free(pool);
        return NULL;
    }


    pthread_mutex_init(pool->qMutex, NULL);
    sem_init(pool->sem, 0, 0);

    logInfo("ThreadPool created");

    return pool;
}

void tPoolFree(tPoolT *tPool) {
    if (!tPool->stopped) {
        return;
    }
    sem_destroy(tPool->sem);
    free(tPool->sem);
    pthread_mutex_destroy(tPool->qMutex);
    free(tPool->qMutex);
    free(tPool->threads);
    queueFree(tPool->queue);
    free(tPool);
}

int tPoolStart(tPoolT *tPool) {
    bool success = true;
    int i;

    for (i = 0; i < tPool->nThreads && success; ++i) {
        routine_args_t *args = new_args(tPool, i);
        if (args == NULL) {
            success = false;
            break;
        }

        if (pthread_create(&tPool->threads[i], NULL, routine, args) != 0) {
            success = false;
        }
    }

    if (!success) {
        for (int j = 0; j < i; ++j) {
            pthread_cancel(tPool->threads[j]);
        }
        logFatal(ERR_FSTR, "Failed to create pthreads", strerror(errno));
        return -1;
    }

    logInfo("ThreadPool started (thread num = %d)", tPool->nThreads);

    return 0;
}

int tPoolAddTask(tPoolT *tPool, taskT *task) {
    pthread_mutex_lock(tPool->qMutex);
    int rc = queuePush(tPool->queue, task);
    sem_post(tPool->sem);
    pthread_mutex_unlock(tPool->qMutex);

    if (rc != 0) {
        logFatal(ERR_FSTR, "tPoolAddTask error", strerror(errno));
        return rc;
    }

    logDebug("task added (q len = %d)", tPool->queue->len);
    return 0;
}

int tPoolStop(tPoolT *tPool) {
    pthread_mutex_lock(tPool->qMutex);
    tPool->stopping = true;
    for (int i = 0; i < tPool->nThreads; ++i) {
        sem_post(tPool->sem);
    }
    pthread_mutex_unlock(tPool->qMutex);
    logInfo("stopping flag is set");

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);

    for (int i = 0; i < tPool->nThreads; ++i) {
        if (tPool->threads[i] != 0) {
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 5;
            int rc = pthread_timedjoin_np(tPool->threads[i], NULL, &timeout);
            logDebug("try to spot thread-%d", i);
            if (rc == ETIMEDOUT) {
                pthread_cancel(tPool->threads[i]);
                logDebug("thread-%d canceled", i);
            } else {
                logDebug("thread-%d joined", i);
            }
        }
    }

    logInfo("all threads stopped");
    tPool->stopped = true;
    return 0;
}

routine_args_t *new_args(tPoolT *pool, int num) {
    routine_args_t *args = calloc(1, sizeof(routine_args_t));
    if (args == NULL) {
        logFatal(ERR_FSTR, "failed to alloc args", strerror(errno));
        return NULL;
    }

    args->num = num;
    args->pool = pool;

    return args;
}

void *routine(void *args) {
    routine_args_t *r_args = (routine_args_t *) args;
    tPoolT *pool = r_args->pool;
    int num = r_args->num;
    free(args);

    taskT task;
    char name[15] = "";
    sprintf(name, "thread-%d", num);
    threadName = name;

    while (true) {
        sem_wait(pool->sem);

        if (pool->stopping) {
            logDebug("stopped");
            break;
        }

        pthread_mutex_lock(pool->qMutex);

        // ===== CRITICAL SECTION =====
        int rc = queuePop(pool->queue, &task);
        if (rc != -1) {
            logDebug("task taken (q len = %d)", pool->queue->len);
        }
        // ============================

        pthread_mutex_unlock(pool->qMutex);

        if (rc < 0) {
            continue;
        }

        task.handler(task.conn, task.wd);
        logInfo("routine for task finished");
    }

    pthread_exit(NULL);
}
