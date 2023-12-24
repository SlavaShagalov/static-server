#pragma once

#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "../tpool/t_pool.h"
#include "../net/net.h"

#define HOST_SIZE 16

typedef struct httpServer {
    char host[HOST_SIZE];
    int port;

    struct pollfd *clients;
    long nClients;

    int listenSock;
    tPoolT *tPool;

    char *wd;
} httpServerT;

httpServerT *httpServerNew(const char *host, int port, int nThreads, const char *wd);

void httpServerFree(httpServerT *server);

int httpServerStart(httpServerT *server);
