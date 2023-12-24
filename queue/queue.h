#pragma once

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "task.h"

typedef struct qNode qNodeT;

struct qNode {
    taskT *task;
    qNodeT *next;
};

typedef struct queue {
    qNodeT *front;
    qNodeT *back;
    int len;
    int totalPop, totalPush;
} queueT;

queueT *queueNew();

void queueFree(queueT *queue);

int queuePush(queueT *queue, taskT *task);

int queuePop(queueT *queue, taskT *task);

int queueEmpty(queueT *queue);
