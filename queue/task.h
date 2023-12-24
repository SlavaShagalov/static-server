#pragma once

#include <poll.h>

typedef void (*handler_t)(int, char *);

typedef struct task {
    handler_t handler;

    int conn;
    char* wd;
} task_t;
