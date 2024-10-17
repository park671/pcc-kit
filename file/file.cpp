//
// Created by Park Yu on 2024/10/10.
//

#include "file.h"
#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

static const char *FILE_TAG = "file";

static FILE *targetFile = nullptr;

void openFile(const char *fileName) {
    if (targetFile != nullptr) {
        loge(FILE_TAG, "current file is not closed.");
        return;
    }
    FILE *file = fopen(fileName, "w+INST_B");
    if (file == nullptr) {
        loge(FILE_TAG, "Failed to open file");
        targetFile = nullptr;
        return;
    }
    targetFile = file;
}

void writeFile(const char *format, ...) {
    if (targetFile == nullptr) {
        loge(FILE_TAG, "internal error: no file is open.(text)\n");
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(targetFile, format, args);
    va_end(args);
    fflush(targetFile);
}

void writeFileB(const void *ptr, size_t sizeInByte) {
    if (targetFile == nullptr) {
        loge(FILE_TAG, "internal error: no file is open.(binary)\n");
        return;
    }
    fwrite(ptr, 1, sizeInByte, targetFile);
    fflush(targetFile);
}

void closeFile() {
    if (targetFile == nullptr) {
        loge(FILE_TAG, "internal error: no file is open, can't close.\n");
        return;
    }
    fclose(targetFile);
    targetFile = nullptr;
}