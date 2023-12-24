#pragma once

#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "../log/log.h"

#define REQ_SIZE 2048
#define RESP_SIZE 1024

int netListen(char *host, int port);

int netAccept(int listen_sock);

ssize_t netWrite(int fd, const void *buf, size_t n);

ssize_t netRead(int fd, void *buf, size_t n);
