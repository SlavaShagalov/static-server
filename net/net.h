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

int listen_net(char *host, int port);

int accept_net(int listen_sock);

ssize_t write_net(int fd, const void *buf, size_t n);

ssize_t read_net(int fd, void *buf, size_t n);
