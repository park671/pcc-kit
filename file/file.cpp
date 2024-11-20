//
// Created by Park Yu on 2024/10/10.
//

#include "file.h"
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include "logger.h"

static const char *FILE_TAG = "file";

// 将 value 对齐到 alignment 的倍数
int alignTo(int value, int alignment) {
    int remainder = value % alignment;
    if (remainder == 0) {
        return value; // 已对齐
    }
    return value + alignment - remainder; // 向上对齐
}

static FILE *targetFile = nullptr;
static volatile uint32_t fileOffset = 0;

void openFile(const char *fileName) {
    if (targetFile != nullptr) {
        loge(FILE_TAG, "current file is not closed.");
        return;
    }
    FILE *file = fopen(fileName, "w+b");
    if (file == nullptr) {
        loge(FILE_TAG, "Failed to open file");
        targetFile = nullptr;
        return;
    }
    targetFile = file;
    fileOffset = 0;
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
    fileOffset += sizeInByte;
}

uint32_t getCurrentFileOffset() {
    return fileOffset;
}

void writeEmptyAlignment(uint32_t alignment) {
    int targetOffset = alignTo(fileOffset, alignment);
    int fillByte = targetOffset - fileOffset;
    if (fillByte == 0) {
        return;
    }
    char *emptyData = (char *) malloc(fillByte);
    memset(emptyData, 0, fillByte);
    writeFileB(emptyData, fillByte);
    if (targetOffset != fileOffset) {
        loge(FILE_TAG, "can not fill empty to alignment:%d: %d->%d", alignment, fileOffset, targetOffset);
        exit(-1);
    }
}

void closeFile() {
    if (targetFile == nullptr) {
        loge(FILE_TAG, "internal error: no file is open, can't close.\n");
        return;
    }
    fclose(targetFile);
    targetFile = nullptr;
}