#pragma once

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <threads.h>

#define ERR_FSTR "%s: %s"

thread_local extern const char *threadName;

typedef enum logLevel {
    FATAL = 0, ERROR, WARN, INFO, DEBUG, TRACE
} logLevelT;

int logInit();

void logFree();

void logSetFd(int fd);

void logSetLevel(logLevelT level);

void logDebug(const char *fstr, ...);

void logInfo(const char *fstr, ...);

void logWarn(const char *fstr, ...);

void logError(const char *fstr, ...);

void logFatal(const char *fstr, ...);

void logTrace(const char *fstr, ...);
