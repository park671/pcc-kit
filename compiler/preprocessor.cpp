//
// Created by Park Yu on 2024/11/25.
//

#include <stdio.h>
#include <string.h>
#include "preprocessor.h"
#include "logger.h"
#include "mspace.h"

const char *PREPROCESSOR_TAG = "preprocessor";
//16KB
#define BUFFER_SIZE 16384

const char *INCLUDE_TAG = "include";
const int INCLUDE_TAG_LEN = 7;
const char *DEFINE_TAG = "define";
const int DEFINE_TAG_LEN = 6;

ProcessedSource *sourceHead = nullptr;

bool processLine(char *line);

bool ignoreComment(const char *line) {
    int length = strlen(line);
    int i = 0;
    for (i = 0; i < length; i++) {
        if (line[i] == ' ') {
            continue;
        }
        break;
    }
    if (line[i] == '/' && i + 1 < length && line[i + 1] == '/') {
        return true;
    }
    return false;
}

void extraInclude(const char *includeFileName) {
    const char *compilerIncludePath = "include/";
    const int finalPathLen = strlen(compilerIncludePath) + strlen(includeFileName);
    char *finalIncludePath = (char *) pccMalloc(PREPROCESSOR_TAG, finalPathLen);
    strcat(finalIncludePath, compilerIncludePath);
    strcat(finalIncludePath, includeFileName);
    FILE *inputFile = fopen(includeFileName, "r");
    if (inputFile == nullptr) {
        inputFile = fopen(finalIncludePath, "r");
    }
    if (inputFile == nullptr) {
        loge(PREPROCESSOR_TAG, "can not found header file: %s", includeFileName);
        exit(-1);
    }
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, inputFile) != nullptr) {
        if (ignoreComment(buffer)) {
            continue;
        }
        if (processLine(buffer)) {
            continue;
        }
        ProcessedSource *p = (ProcessedSource *) (pccMalloc(PREPROCESSOR_TAG, sizeof(ProcessedSource)));
        int lineLength = strlen(buffer);
        p->line = (char *) pccMalloc(PREPROCESSOR_TAG, lineLength);
        p->next = nullptr;
        memcpy((void *) p->line, buffer, lineLength);
        p->length = lineLength;
        //linked list
        if (sourceHead == nullptr) {
            sourceHead = p;
        } else {
            ProcessedSource *tail = sourceHead;
            while (tail->next != nullptr) {
                tail = tail->next;
            }
            tail->next = p;
        }
    }
    fclose(inputFile);
}

bool processLine(char *line) {
    int length = strlen(line);
    bool skipTrim = true;
    bool isMacro = false;
    for (int i = 0; i < length; i++) {
        if (line[i] == ' ' && skipTrim) {
            continue;
        }
        skipTrim = false;
        if (isMacro) {
            bool isInclude = true;
            for (int j = 0; j < INCLUDE_TAG_LEN; j++) {
                if (line[j + i] != INCLUDE_TAG[j]) {
                    isInclude = false;
                    break;
                }
            }
            if (isInclude) {
                int startIdx = -1;
                int endIdx = -1;
                for (int j = i + INCLUDE_TAG_LEN; j < length; j++) {
                    if (line[j] == '<') {
                        startIdx = j + 1;
                    } else if (line[j] == '>') {
                        endIdx = j - 1;
                    }
                }
                if (startIdx == -1 || endIdx == -1 || endIdx - startIdx < 2) {
                    loge(PREPROCESSOR_TAG, "invalid #include: %s", line);
                    exit(-1);
                }
                int includeFileLen = endIdx - startIdx + 1;
                char *includeFileName = (char *) pccMalloc(PREPROCESSOR_TAG, includeFileLen);
                memcpy(includeFileName, line + startIdx, includeFileLen);
                logd(PREPROCESSOR_TAG, "find include file: \"%s\"", includeFileName);
                extraInclude(includeFileName);
                return true;
            }
        } else if (line[i] == '#') {
            isMacro = true;
        } else {
            break;
        }
    }
    return false;
}

ProcessedSource *preprocess(const char *sourceFilePath) {
    logd(PREPROCESSOR_TAG, "preprocess...");
    FILE *inputFile = fopen(sourceFilePath, "r");
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, inputFile) != nullptr) {
        if (ignoreComment(buffer)) {
            continue;
        }
        if (processLine(buffer)) {
            continue;
        }
        ProcessedSource *p = (ProcessedSource *) (pccMalloc(PREPROCESSOR_TAG, sizeof(ProcessedSource)));
        int lineLength = strlen(buffer);
        p->line = (char *) pccMalloc(PREPROCESSOR_TAG, lineLength);
        p->next = nullptr;
        memcpy((void *) p->line, buffer, lineLength);
        p->length = lineLength;
        //linked list
        if (sourceHead == nullptr) {
            sourceHead = p;
        } else {
            ProcessedSource *tail = sourceHead;
            while (tail->next != nullptr) {
                tail = tail->next;
            }
            tail->next = p;
        }
    }
    fclose(inputFile);
    return sourceHead;
}

extern void releasePreProcessorMemory() {
    pccFreeSpace(PREPROCESSOR_TAG);
}