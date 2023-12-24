#pragma once

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <threads.h>

#define ERR_FSTR "%s: %s"

thread_local extern const char *thread_name;

typedef enum log_level_t {
    FATAL = 0, ERROR, WARN, INFO, DEBUG, TRACE
} log_level_t;

int log_init();

void log_free();

void set_log_fd(int fd);

void set_log_level(log_level_t level);

void log_debug(const char *fstr, ...);

void log_info(const char *fstr, ...);

void log_warn(const char *fstr, ...);

void log_error(const char *fstr, ...);

void log_fatal(const char *fstr, ...);

void log_trace(const char *fstr, ...);
