#include "server.h"
#include "request.h"
#include "responses.h"
#include "content_type.h"

#define PATH_MAX 256
#define HEADER_LEN 128

void handle_connection(int clientfd, char *wd);

taskT *new_task_t(handlerT handler, int conn, char *wd);

int read_req(char *buff, int clientfd);

void send_resp(char *path, int client_fd, request_method_t type);

void send_err(int clientfd, const char *str);

void process_req(int clientfd, requestT *req, char *wd);

bool is_prefix(char *prefix, char *str);

void send_file(char *path, int clientfd);

void process_get_req(char *path, int clientfd);

void process_head_req(char *path, int clientfd);

int send_headers(char *path, int clientfd);

char *get_type(char *path);

char *get_content_type(char *path);

httpServerT *httpServerNew(const char *host, int port, int nThreads, const char *wd) {
    if (chdir(wd)) {
        logFatal(ERR_FSTR, "Failed to change work dir", strerror(errno));
        return NULL;
    }

    httpServerT *server = calloc(1, sizeof(httpServerT));
    if (server == NULL) {
        return NULL;
    }

    strcpy(server->host, host);
    server->port = port;

    server->nClients = sysconf(_SC_OPEN_MAX);
    if (server->nClients < 0) {
        logFatal(ERR_FSTR, "Failed to get max client num", strerror(errno));
        free(server);
        return NULL;
    }

    server->clients = malloc(sizeof(struct pollfd) * server->nClients);
    if (server->clients == NULL) {
        logFatal(ERR_FSTR, "Failed to alloc clients fds", strerror(errno));
        free(server);
        return NULL;
    }

    for (long i = 0; i < server->nClients; ++i) {
        server->clients[i].fd = -1;
    }

    server->tPool = tPoolNew(nThreads);
    if (server->tPool == NULL) {
        free(server->clients);
        free(server);
        return NULL;
    }

    server->wd = calloc(PATH_MAX, sizeof(char));
    if (server->wd == NULL) {
        logFatal(ERR_FSTR, "Failed to alloc wd", strerror(errno));
        tPoolFree(server->tPool);
        free(server->clients);
        free(server);
        return NULL;
    }

    if (getcwd(server->wd, PATH_MAX) == NULL) {
        logFatal(ERR_FSTR, "Failed to get wd", strerror(errno));
        free(server->wd);
        tPoolFree(server->tPool);
        free(server->clients);
        free(server);
        return NULL;
    }

    logInfo("Server created");
    return server;
}

void httpServerFree(httpServerT *server) {
    server->clients[0].revents = -1;
    close(server->listenSock);

    tPoolStop(server->tPool);
    tPoolFree(server->tPool);

    free(server->wd);
    free(server->clients);
    free(server);

    logInfo("server destroyed");
}

int httpServerStart(httpServerT *server) {
    logInfo("Server starting");
    logInfo("Host: %s", server->host);
    logInfo("Port: %d", server->port);
    logInfo("Work dir: %s", server->wd);

    server->listenSock = netListen(server->host, server->port);
    if (server->listenSock < 0) {
        return -1;
    }

    if (tPoolStart(server->tPool) != 0) {
        return -1;
    }

    long max_i = 0, n_ready;
    server->clients[0].fd = server->listenSock;
    server->clients[0].events = POLLIN;

    while (true) {
        n_ready = poll(server->clients, max_i + 1, -1);
        if (n_ready < 0) {
            logFatal(ERR_FSTR, "poll error", strerror(errno));
            return -1;
        }

        if (server->clients[0].revents & POLLIN) {
            int client_sock = netAccept(server->listenSock);
            if (client_sock < 0) {
                continue;
            }

            long i = 0;
            for (i = 1; i < server->nClients; ++i) {
                if (server->clients[i].fd < 0) {
                    server->clients[i].fd = client_sock;
                    break;
                }
            }
            if (i == server->nClients) {
                logError("too many connections");
                continue;
            }
            server->clients[i].events = POLLIN;

            if (i > max_i) {
                max_i = i;
            }
            if (--n_ready <= 0) {
                continue;
            }
        }

        for (int i = 1; i <= max_i; ++i) {
            if (server->clients[i].fd < 0) {
                continue;
            }

            if (server->clients[i].revents & (POLLIN | POLLERR)) {
                taskT *task = new_task_t(handle_connection, server->clients[i].fd, server->wd);
                if (task == NULL) {
                    continue;
                }

                tPoolAddTask(server->tPool, task);
                server->clients[i].fd = -1;

                if (--n_ready <= 0) {
                    break;
                }
            }
        }
    }
}

void handle_connection(int clientfd, char *wd) {
    requestT req;
    char *buff = calloc(REQ_SIZE, sizeof(char));
    if (buff == NULL) {
        logError(ERR_FSTR, "failed alloc req buf", strerror(errno));
        return;
    }

    logDebug("handle_connection started");
    if (read_req(buff, clientfd) < 0) {
        send_err(clientfd, INT_SERVER_ERR_STR);
        close(clientfd);
        free(buff);
        return;
    }
    logDebug("read_req");

    if (parse_req(&req, buff) < 0) {
        send_err(clientfd, BAD_REQUEST_STR);
        close(clientfd);
        free(buff);
        return;
    }
    if (req.method == BAD) {
        logError("unsupported http method");
        send_err(clientfd, M_NOT_ALLOWED_STR);
        close(clientfd);
        free(buff);
        return;
    }

    process_req(clientfd, &req, wd);

    close(clientfd);
    logDebug("handle_connection finished");

    free(buff);
}

taskT *new_task_t(handlerT handler, int conn, char *wd) {
    taskT *task = calloc(1, sizeof(taskT));
    if (task == NULL) {
        logError(ERR_FSTR, "task alloc failed", strerror(errno));
        return NULL;
    }
    task->wd = wd;
    task->conn = conn;
    task->handler = handler;
    return task;
}

int read_req(char *buff, int clientfd) {
    logDebug("read_req in");
    long byte_read = netRead(clientfd, buff, REQ_SIZE - 1);
    if (byte_read <= 0) {
        return -1;
    }

    buff[byte_read - 1] = '\0';
    logDebug("%s", buff);

    return 0;
}

void process_req(int clientfd, requestT *req, char *wd) {
    char *path = calloc(PATH_MAX, sizeof(char));
    if (path == NULL) {
        logError(ERR_FSTR, "failed alloc path buf", strerror(errno));
        send_err(clientfd, INT_SERVER_ERR_STR);
        return;
    }

    if (realpath(req->url, path) == NULL) {
        if (errno == ENOENT) {
            send_err(clientfd, NOT_FOUND_STR);
        } else {
            send_err(clientfd, INT_SERVER_ERR_STR);
        }
        logError(ERR_FSTR, "realpath error", strerror(errno));
        free(path);
        return;
    }
    logDebug("path: %s", path);

    if (!is_prefix(wd, path)) {
        send_err(clientfd, FORBIDDEN_STR);
        logError("attempt to access outside the root");
        free(path);
        return;
    }

    send_resp(req->url, clientfd, GET);
    free(path);
}

bool is_prefix(char *prefix, char *str) {
    while (*prefix && *str && *prefix++ == *str++);

    if (*prefix == '\0') {
        return true;
    }
    return false;
}

void send_resp(char *path, int client_fd, request_method_t type) {
    switch (type) {
        case GET:
            process_get_req(path, client_fd);
            break;
        case HEAD:
            process_head_req(path, client_fd);
            break;
        default:
            logError("unsupported http method");
            send_err(client_fd, M_NOT_ALLOWED_STR);
            break;
    }
}

void process_get_req(char *path, int clientfd) {
    logDebug("process as GET");
    if (send_headers(path, clientfd) < 0) {
        return;
    }

    send_file(path, clientfd);
}

void process_head_req(char *path, int clientfd) {
    logDebug("process as HEAD");
    send_headers(path, clientfd);
}

int send_headers(char *path, int clientfd) {
    char status[] = OK_STR;
    char connection[] = "Connection: close";

    char *len = calloc(HEADER_LEN, sizeof(char));
    char *type = calloc(HEADER_LEN, sizeof(char));
    if (len == NULL || type == NULL) {
        logError(ERR_FSTR, "failed to alloc headers buffs", strerror(errno));
        send_err(clientfd, INT_SERVER_ERR_STR);
        return -1;
    }

    struct stat st;
    if (stat(path, &st) < 0) {
        send_err(clientfd, NOT_FOUND_STR);
        perror("stat error");
        free(len);
        free(type);
        return -1;
    }

    sprintf(len, "Content-Length: %ld", st.st_size);
    char *mime_type = get_content_type(path);

    int rc = 0;
    char *res_str;
    if (mime_type == NULL) {
        logWarn("could not determine the file type");
        rc = asprintf(&res_str, "%s\r\n%s\r\n%s\r\n\r\n", status, connection, len);
    } else {
        sprintf(type, "Content-Type: %s", mime_type);
        rc = asprintf(&res_str, "%s\r\n%s\r\n%s\r\n%s\r\n\r\n", status, connection, len, type);
    }
    if (rc < 0) {
        logError("formation of headers of http response failed");
        send_err(clientfd, INT_SERVER_ERR_STR);
        free(len);
        free(type);
        return -1;
    }

    ssize_t byte_write = netWrite(clientfd, res_str, rc);
    if (byte_write < 0) {
        free(res_str);
        free(len);
        free(type);
        return -1;
    }

    logDebug("headers: %s", res_str);
    logDebug("send %d bytes", byte_write);

    free(res_str);
    free(len);
    free(type);
    return 0;
}

void send_file(char *path, int clientfd) {
    int fd = open(path, 0);
    if (fd < 0) {
        logError(ERR_FSTR, "open error", strerror(errno));
        send_err(clientfd, NOT_FOUND_STR);
        return;
    }

    char *buff_resp = calloc(RESP_SIZE, sizeof(char));
    if (buff_resp == NULL) {
        logError(ERR_FSTR, "failed alloc resp buf", strerror(errno));
        send_err(clientfd, INT_SERVER_ERR_STR);
        return;
    }
    unsigned long long total_read = 0, total_write = 0;
    long byte_write = 0, byte_read = 0;

    while ((byte_read = netRead(fd, buff_resp, RESP_SIZE)) > 0) {
        total_read += byte_read;

        byte_write = write(clientfd, buff_resp, byte_read);
        if (byte_write < 0) {
            logError(ERR_FSTR, "write error", strerror(errno));
            break;
        }
        total_write += byte_write;
    }
    logDebug("total read %llu bytes", total_read);
    logDebug("total sent %llu bytes", total_write);

    close(fd);
    logInfo("successful response");

    free(buff_resp);
}

void send_err(int clientfd, const char *str) {
    logInfo("%s", str);
    netWrite(clientfd, str, strlen(str));
}

char *get_type(char *path) {
    char *res = path + strlen(path) - 1;
    while (res >= path && *res != '.' && *res != '/') {
        res--;
    }

    if (res < path || *res == '/') {
        return NULL;
    }
    return ++res;
}

char *get_content_type(char *path) {
    char *ext = get_type(path);
    if (ext == NULL) {
        return NULL;
    }

    int i = 0;
    for (; i < TYPE_NUM && strcmp(TYPE_EXT[i], ext) != 0; i++);
    if (i >= TYPE_NUM) {
        return NULL;
    }

    return MIME_TYPE[i];
}
