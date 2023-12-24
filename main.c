#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "constants.h"
#include "server/server.h"

http_server_t *server = NULL;

void sig_handler(int signum) {
    printf("received signal %d\n", signum);
    free_http_server_t(server);
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    printf("pid: %d\n", getpid());

    // Logger
    if (log_init() < 0) {
        return -1;
    }
    set_log_level(INFO);

    // Server
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    server = new_http_server(ip, port, n_threads, wd);
    if(server == NULL) {
        return -1;
    }
    if (run_http_server_t(server) < 0) {
        free_http_server_t(server);
    }

    return 0;
}
