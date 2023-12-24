#include "log.h"

#define STDOUT_FD 1

thread_local const char *thread_name = "main";

const char *log_level_str[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

typedef struct logger {
    log_level_t level;
    int fd;
    pthread_mutex_t *mutex;
} logger_t;

logger_t logger;

void print(const char *fstr, log_level_t level, va_list argptr);

int print_file(const char *fstr, log_level_t level, va_list argptr);

void print_stdout(const char *fstr, log_level_t level, va_list argptr);

const char *ltostr(log_level_t);

int log_init() {
    logger.fd = STDOUT_FD;
    logger.level = INFO;

    logger.mutex = calloc(1, sizeof(pthread_mutex_t));
    if (logger.mutex == NULL) {
        printf("log init failed: mutex allocate failed\n");
        return -1;
    }

    pthread_mutex_init(logger.mutex, NULL);

    return 0;
}

void log_free() {
    free(logger.mutex);
}

void set_log_fd(int fd) {
    logger.fd = fd;
}

void set_log_level(log_level_t level) {
    logger.level = level;
}

void log_debug(const char *fstr, ...) {
    if (logger.level >= DEBUG) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, DEBUG, argptr);
        va_end(argptr);
    }
}

void log_info(const char *fstr, ...) {
    if (logger.level >= INFO) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, INFO, argptr);
        va_end(argptr);
    }
}

void log_warn(const char *fstr, ...) {
    if (logger.level >= WARN) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, WARN, argptr);
        va_end(argptr);
    }
}

void log_error(const char *fstr, ...) {
    if (logger.level >= ERROR) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, ERROR, argptr);
        va_end(argptr);
    }
}

void log_fatal(const char *fstr, ...) {
    if (logger.level >= FATAL) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, FATAL, argptr);
        va_end(argptr);
    }
}

void log_trace(const char *fstr, ...) {
    if (logger.level >= TRACE) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, TRACE, argptr);
        va_end(argptr);
    }
}

void print(const char *fstr, log_level_t level, va_list argptr) {
    if (print_file(fstr, level, argptr) < 0) {
        print_stdout(fstr, level, argptr);
    }
}

int print_file(const char *fstr, log_level_t level, va_list argptr) {

    time_t t = time(NULL);
    struct tm local;
    localtime_r(&t, &local);

    pthread_mutex_lock(logger.mutex);
    if (dprintf(logger.fd, "[%02d/%02d/%dT%02d:%02d:%02d] --- [%s] --- [%s] --- ", local.tm_mday, local.tm_mon + 1,
                local.tm_year + 1900, local.tm_hour, local.tm_min, local.tm_sec, thread_name,
                ltostr(level)) < 0) {
        pthread_mutex_unlock(logger.mutex);
        return -1;
    }
    if (vdprintf(logger.fd, fstr, argptr) < 0) {
        pthread_mutex_unlock(logger.mutex);
        return -1;
    }
    int rc = dprintf(logger.fd, "\n");
    pthread_mutex_unlock(logger.mutex);

    return rc;
}

void print_stdout(const char *fstr, log_level_t level, va_list argptr) {
    time_t t = time(NULL);
    struct tm *local = localtime(&t);

    pthread_mutex_lock(logger.mutex);
    printf("[%02d/%02d/%dT%02d:%02d:%02d] --- [%s] --- [%s]\t", local->tm_mday, local->tm_mon + 1,
           local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec, thread_name,
           ltostr(level));
    vprintf(fstr, argptr);
    pthread_mutex_unlock(logger.mutex);
}

const char *ltostr(log_level_t level) {
    return log_level_str[level];
}
