//
// Created by Park Yu on 2024/9/11.
//

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

char *get_current_time_str() {
    static char buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *local = localtime(&tv.tv_sec);

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             local->tm_year + 1900,
             local->tm_mon + 1,
             local->tm_mday,
             local->tm_hour,
             local->tm_min,
             local->tm_sec,
             tv.tv_usec / 1000);

    return buffer;
}

void logd(const char *tag, const char *fmt, ...) {
    if (!ENABLE_LOGGER) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "%s - %s:\t\t", get_current_time_str(), tag);
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
    fprintf(stderr, "%s - %s:\t\t", get_current_time_str(), tag);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}