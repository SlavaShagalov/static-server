#pragma once

#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "../tpool/tpool.h"
#include "../net/net.h"

#define HOST_SIZE 16

typedef struct http_server {
    char host[HOST_SIZE];
    int port;

    struct pollfd *clients;
    long cl_num;

    int listen_sock;
    tpool_t *pool;

    char *wd;
} http_server_t;

http_server_t *new_http_server(const char *host, int port, int thread_num, const char *wd);

void free_http_server_t(http_server_t *server);

int run_http_server_t(http_server_t *server);
