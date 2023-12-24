#include "queue.h"
#include "../log/log.h"

q_node_t *new_q_node_t(task_t *task);

void free_q_node_t(q_node_t *node);

task_t *extract_q_node_t(q_node_t *node);

queue_t *new_queue_t() {
    return calloc(1, sizeof(queue_t));
}

void free_queue_t(queue_t *queue) {
    q_node_t *cur = queue->front;
    while (cur != NULL) {
        q_node_t *next = cur->next;
        free_q_node_t(cur);
        cur = next;
    }
    free(queue);
}

int push(queue_t *queue, task_t *task) {
    q_node_t *node = new_q_node_t(task);
    if (node == NULL) return -1;

    if (queue->back == NULL) {
        queue->front = node;
        queue->back = node;
    } else {
        queue->back->next = node;
        queue->back = node;
    }

    queue->len++;
    queue->t_push++;
    log_info("task pushed to queue (len = %d, total = %d)", queue->len, queue->t_push);

    return 0;
}

int pop(queue_t *queue, task_t *task) {
    if (queue->front == NULL) {
        log_warn("pop from empty queue (len = %d)", queue->len);
        return -1;
    }

    q_node_t *node = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->back = NULL;
    }

    queue->len--;
    queue->t_get++;
    log_info("task extracted from queue (len = %d, total = %d)", queue->len, queue->t_get);

    task_t *tmp_task = extract_q_node_t(node);
    memcpy(task, tmp_task, sizeof(task_t));
    free(tmp_task);

    return 0;
}

int empty(queue_t *queue) {
    return queue->front == NULL;
}

q_node_t *new_q_node_t(task_t *task) {
    q_node_t *node = calloc(1, sizeof(q_node_t));
    if (node == NULL) {
        log_error(ERR_FSTR, "node alloc failed", strerror(errno));
        return NULL;
    }

    node->task = task;

    return node;
}

void free_q_node_t(q_node_t *node) {
    free(node->task);
    free(node);
}

task_t *extract_q_node_t(q_node_t *node) {
    task_t *task = node->task;
    free(node);
    return task;
}
