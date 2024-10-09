//
// Created by Park Yu on 2024/9/11.
//

#ifndef MICRO_CC_LOGGER_H
#define MICRO_CC_LOGGER_H

#define ENABLE_LOGGER true

void logd(const char *tag, const char *fmt, ...);

void loge(const char *tag, const char *fmt, ...);

#endif //MICRO_CC_LOGGER_H
