//
// Created by Park Yu on 2024/11/25.
//

#ifndef PCC_PREPROCESSOR_H
#define PCC_PREPROCESSOR_H

struct ProcessedSource {
    char *line;
    int length;
    ProcessedSource *next;
};

extern ProcessedSource *preprocess(const char *sourceFilePath);
extern void releasePreProcessorMemory();

#endif //PCC_PREPROCESSOR_H
