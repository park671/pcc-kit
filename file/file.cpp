//
// Created by Park Yu on 2024/10/10.
//

#include "file.h"
#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

static const char *FILE_TAG = "file";

FILE *assemblyFile = nullptr;

void openFile(const char *fileName) {
    if (assemblyFile != nullptr) {
        loge(FILE_TAG, "current file is not closed.");
        return;
    }
    FILE *file = fopen(fileName, "w+");
    if (file == nullptr) {
        perror("Failed to open file");
        assemblyFile = nullptr;
    }
    assemblyFile = file;
}

void writeFile(const char *format, ...) {
    if (assemblyFile == nullptr) {
        loge(FILE_TAG, "internal error: no file is open.\n");
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(assemblyFile, format, args);
    va_end(args);
    fflush(assemblyFile);
}

void closeFile() {
    if (assemblyFile == nullptr) {
        loge(FILE_TAG, "internal error: no file is open.\n");
        return;
    }
    fclose(assemblyFile);
    assemblyFile = nullptr;
}