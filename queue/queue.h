#pragma once

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "task.h"

typedef struct q_node q_node_t;

struct q_node {
    task_t *task;
    q_node_t *next;
};

typedef struct queue {
    q_node_t *front;
    q_node_t *back;
    int len;
    int t_get, t_push;
} queue_t;

queue_t *new_queue_t();

void free_queue_t(queue_t *queue);

int push(queue_t *queue, task_t *task);

int pop(queue_t *queue, task_t *task);

int empty(queue_t *queue);
