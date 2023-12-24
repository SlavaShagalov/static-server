#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "constants.h"
#include "server/server.h"

httpServerT *server = NULL;

void sigHandler(int signum) {
    printf("received signal %d\n", signum);
    httpServerFree(server);
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    printf("pid: %d\n", getpid());

    // Logger
    if (logInit() < 0) {
        return -1;
    }
    logSetLevel(INFO);

    // Server
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGKILL, sigHandler);
    signal(SIGHUP, sigHandler);
    signal(SIGPIPE, SIG_IGN);

    server = httpServerNew(ipAddress, port, nThreads, wd);
    if(server == NULL) {
        return -1;
    }
    if (httpServerStart(server) < 0) {
        httpServerFree(server);
    }

    return 0;
}
