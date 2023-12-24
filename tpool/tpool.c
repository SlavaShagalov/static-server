#include "tpool.h"

typedef struct routine_args {
    tpool_t *pool;
    int num;
} routine_args_t;

void *routine(void *routine_args);

routine_args_t *new_args(tpool_t *pool, int num);

tpool_t *new_tpool_t(int thread_num) {
    tpool_t *pool = calloc(1, sizeof(tpool_t));
    if (pool == NULL) {
        log_fatal(ERR_FSTR, "pool alloc failed", strerror(errno));
        return NULL;
    }

    pool->queue = new_queue_t();
    if (pool->queue == NULL) {
        log_fatal(ERR_FSTR, "queue alloc failed", strerror(errno));
        free(pool);
        return NULL;
    }
    pool->queue->len = 0;

    pool->th = calloc(thread_num, sizeof(pthread_t));
    if (pool->th == NULL) {
        log_fatal(ERR_FSTR, "pthreads alloc failed", strerror(errno));
        free_queue_t(pool->queue);
        free(pool);
        return NULL;
    }
    pool->t_sz = thread_num;

    pool->q_mutex = calloc(1, sizeof(pthread_mutex_t));
    if (pool->q_mutex == NULL) {
        log_fatal(ERR_FSTR, "mutex alloc failed", strerror(errno));
        free_queue_t(pool->queue);
        free(pool->th);
        free(pool);
        return NULL;
    }

    pool->sem = calloc(1, sizeof(sem_t));
    if (pool->sem == NULL) {
        log_fatal(ERR_FSTR, "cond var alloc failed", strerror(errno));
        free_queue_t(pool->queue);
        free(pool->th);
        free(pool->q_mutex);
        free(pool);
        return NULL;
    }


    pthread_mutex_init(pool->q_mutex, NULL);
    sem_init(pool->sem, 0, 0);

    log_info("tpool created");

    return pool;
}

void free_tpool_t(tpool_t *pool) {
    if (!pool->stopped) {
        return;
    }
    sem_destroy(pool->sem);
    free(pool->sem);
    pthread_mutex_destroy(pool->q_mutex);
    free(pool->q_mutex);
    free(pool->th);
    free_queue_t(pool->queue);
    free(pool);
}

int run_tpool_t(tpool_t *pool) {
    bool success = true;
    int i;

    for (i = 0; i < pool->t_sz && success; ++i) {
        routine_args_t *args = new_args(pool, i);
        if (args == NULL) {
            success = false;
            break;
        }

        if (pthread_create(&pool->th[i], NULL, routine, args) != 0) {
            success = false;
        }
    }

    if (!success) {
        for (int j = 0; j < i; ++j) {
            pthread_cancel(pool->th[j]);
        }
        log_fatal(ERR_FSTR, "failed to create pthreads", strerror(errno));
        return -1;
    }

    log_info("tpool started (thread num = %d)", pool->t_sz);

    return 0;
}

int add_task(tpool_t *pool, task_t *task) {
    pthread_mutex_lock(pool->q_mutex);
    int rc = push(pool->queue, task);
    sem_post(pool->sem);
    pthread_mutex_unlock(pool->q_mutex);

    if (rc != 0) {
        log_fatal(ERR_FSTR, "add_task error", strerror(errno));
        return rc;
    }

    log_debug("task added (q len = %d)", pool->queue->len);
    return 0;
}

int stop(tpool_t *pool) {
    pthread_mutex_lock(pool->q_mutex);
    pool->stop = true;
    for (int i = 0; i < pool->t_sz; ++i) {
        sem_post(pool->sem);
    }
    pthread_mutex_unlock(pool->q_mutex);
    log_info("stop flag is set");

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);

    for (int i = 0; i < pool->t_sz; ++i) {
        if (pool->th[i] != 0) {
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 5;
            int rc = pthread_timedjoin_np(pool->th[i], NULL, &timeout);
            log_debug("try to spot thread-%d", i);
            if (rc == ETIMEDOUT) {
                pthread_cancel(pool->th[i]);
                log_debug("thread-%d canceled", i);
            } else {
                log_debug("thread-%d joined", i);
            }
        }
    }

    log_info("all threads stopped");
    pool->stopped = true;
    return 0;
}

routine_args_t *new_args(tpool_t *pool, int num) {
    routine_args_t *args = calloc(1, sizeof(routine_args_t));
    if (args == NULL) {
        log_fatal(ERR_FSTR, "failed to alloc args", strerror(errno));
        return NULL;
    }

    args->num = num;
    args->pool = pool;

    return args;
}

void *routine(void *args) {
    routine_args_t *r_args = (routine_args_t *) args;
    tpool_t *pool = r_args->pool;
    int num = r_args->num;
    free(args);

    task_t task;
    char name[15] = "";
    sprintf(name, "thread-%d", num);
    thread_name = name;

    while (true) {
        sem_wait(pool->sem);

        if (pool->stop) {
            log_debug("stopped");
            break;
        }

        pthread_mutex_lock(pool->q_mutex);

        // ===== CRITICAL SECTION =====
        int rc = pop(pool->queue, &task);
        if (rc != -1) {
            log_debug("task taken (q len = %d)", pool->queue->len);
        }
        // ============================

        pthread_mutex_unlock(pool->q_mutex);

        if (rc < 0) {
            continue;
        }

        task.handler(task.conn, task.wd);
        log_info("routine for task finished");
    }

    pthread_exit(NULL);
}
