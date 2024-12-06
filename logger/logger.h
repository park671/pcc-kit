//
// Created by Park Yu on 2024/9/11.
//

#ifndef PCC_CC_LOGGER_H
#define PCC_CC_LOGGER_H

#define ENABLE_LOGGER true

void initLogger();

void logd(const char *tag, const char *fmt, ...);

void loge(const char *tag, const char *fmt, ...);

#endif //PCC_CC_LOGGER_H
