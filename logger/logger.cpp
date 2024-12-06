//
// Created by Park Yu on 2024/9/11.
//

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static volatile long firstTs = 0;

void initLogger() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    firstTs = ts.tv_sec;
}

void printCurrentTime() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long ms = ts.tv_nsec / 1000000;
    long ns = (ts.tv_nsec / 1000) % 1000;
    printf("[%lds %03ldms %03ldns] ", (ts.tv_sec - firstTs), ms, ns);
}

void logd(const char *tag, const char *fmt, ...) {
    if (!ENABLE_LOGGER) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    printCurrentTime();
    fprintf(stdout, "%s:\t\t", tag);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void loge(const char *tag, const char *fmt, ...) {
    if (!ENABLE_LOGGER) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s:\t\t", tag);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}