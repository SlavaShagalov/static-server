#include "log.h"

#define STDOUT_FD 1

thread_local const char *threadName = "main";

const char *log_level_str[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

typedef struct logger {
    logLevelT level;
    int fd;
    pthread_mutex_t *mutex;
} logger_t;

logger_t logger;

void print(const char *fstr, logLevelT level, va_list argptr);

int print_file(const char *fstr, logLevelT level, va_list argptr);

void print_stdout(const char *fstr, logLevelT level, va_list argptr);

const char *ltostr(logLevelT);

int logInit() {
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

void logFree() {
    free(logger.mutex);
}

void logSetFd(int fd) {
    logger.fd = fd;
}

void logSetLevel(logLevelT level) {
    logger.level = level;
}

void logDebug(const char *fstr, ...) {
    if (logger.level >= DEBUG) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, DEBUG, argptr);
        va_end(argptr);
    }
}

void logInfo(const char *fstr, ...) {
    if (logger.level >= INFO) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, INFO, argptr);
        va_end(argptr);
    }
}

void logWarn(const char *fstr, ...) {
    if (logger.level >= WARN) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, WARN, argptr);
        va_end(argptr);
    }
}

void logError(const char *fstr, ...) {
    if (logger.level >= ERROR) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, ERROR, argptr);
        va_end(argptr);
    }
}

void logFatal(const char *fstr, ...) {
    if (logger.level >= FATAL) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, FATAL, argptr);
        va_end(argptr);
    }
}

void logTrace(const char *fstr, ...) {
    if (logger.level >= TRACE) {
        va_list argptr;
        va_start(argptr, fstr);
        print(fstr, TRACE, argptr);
        va_end(argptr);
    }
}

void print(const char *fstr, logLevelT level, va_list argptr) {
//    if (print_file(fstr, level, argptr) < 0) {
    print_stdout(fstr, level, argptr);
//    }
}

int print_file(const char *fstr, logLevelT level, va_list argptr) {

    time_t t = time(NULL);
    struct tm local;
    localtime_r(&t, &local);

    pthread_mutex_lock(logger.mutex);
    if (dprintf(logger.fd, "[%02d/%02d/%dT%02d:%02d:%02d] --- [%s] --- [%s] --- ", local.tm_mday, local.tm_mon + 1,
                local.tm_year + 1900, local.tm_hour, local.tm_min, local.tm_sec, threadName,
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

void print_stdout(const char *fstr, logLevelT level, va_list argptr) {
    time_t t = time(NULL);
    struct tm *local = localtime(&t);

    pthread_mutex_lock(logger.mutex);
    if (level == ERROR) {
        printf("%02d-%02d-%d %02d:%02d:%02d \033[1;31m%-5s\033[0m %s ",
               local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
               local->tm_hour, local->tm_min, local->tm_sec,
               ltostr(level), threadName);
    } else if (level == INFO) {
        printf("%02d-%02d-%d %02d:%02d:%02d \033[0;34m%-5s\033[0m %s ",
               local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
               local->tm_hour, local->tm_min, local->tm_sec,
               ltostr(level), threadName);
    } else if (level == WARN) {
        printf("%02d-%02d-%d %02d:%02d:%02d \033[0;33m%-5s\033[0m %s ",
               local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
               local->tm_hour, local->tm_min, local->tm_sec,
               ltostr(level), threadName);
    } else {
        printf("%02d-%02d-%d %02d:%02d:%02d %-5s %s ",
               local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
               local->tm_hour, local->tm_min, local->tm_sec,
               ltostr(level), threadName);
    }
    vprintf(fstr, argptr);
    printf("\n");
    pthread_mutex_unlock(logger.mutex);
}

const char *ltostr(logLevelT level) {
    return log_level_str[level];
}
