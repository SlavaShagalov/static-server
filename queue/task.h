#pragma once

#include <poll.h>

typedef void (*handlerT)(int, char *);

typedef struct task {
    handlerT handler;

    int conn;
    char* wd;
} taskT;
