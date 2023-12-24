#include "queue.h"
#include "../log/log.h"

qNodeT *new_q_node_t(taskT *task);

void free_q_node_t(qNodeT *node);

taskT *extract_q_node_t(qNodeT *node);

queueT *queueNew() {
    return calloc(1, sizeof(queueT));
}

void queueFree(queueT *queue) {
    qNodeT *cur = queue->front;
    while (cur != NULL) {
        qNodeT *next = cur->next;
        free_q_node_t(cur);
        cur = next;
    }
    free(queue);
}

int queuePush(queueT *queue, taskT *task) {
    qNodeT *node = new_q_node_t(task);
    if (node == NULL) return -1;

    if (queue->back == NULL) {
        queue->front = node;
        queue->back = node;
    } else {
        queue->back->next = node;
        queue->back = node;
    }

    queue->len++;
    queue->totalPush++;
    logInfo("task pushed to queue (len = %d, total = %d)", queue->len, queue->totalPush);

    return 0;
}

int queuePop(queueT *queue, taskT *task) {
    if (queue->front == NULL) {
        logWarn("queuePop from queueEmpty queue (len = %d)", queue->len);
        return -1;
    }

    qNodeT *node = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->back = NULL;
    }

    queue->len--;
    queue->totalPop++;
    logInfo("task extracted from queue (len = %d, total = %d)", queue->len, queue->totalPop);

    taskT *tmp_task = extract_q_node_t(node);
    memcpy(task, tmp_task, sizeof(taskT));
    free(tmp_task);

    return 0;
}

int queueEmpty(queueT *queue) {
    return queue->front == NULL;
}

qNodeT *new_q_node_t(taskT *task) {
    qNodeT *node = calloc(1, sizeof(qNodeT));
    if (node == NULL) {
        logError(ERR_FSTR, "node alloc failed", strerror(errno));
        return NULL;
    }

    node->task = task;

    return node;
}

void free_q_node_t(qNodeT *node) {
    free(node->task);
    free(node);
}

taskT *extract_q_node_t(qNodeT *node) {
    taskT *task = node->task;
    free(node);
    return task;
}
