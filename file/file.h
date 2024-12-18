//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_CC_FILE_H
#define PCC_CC_FILE_H

#include <stdio.h>
#include <stdint.h>

uint64_t alignTo(uint64_t value, uint64_t alignment);
uint64_t alignDownTo(uint64_t value, uint64_t alignment);

extern void openFile(const char *fileName);

extern void writeFile(const char *format, ...);

extern void writeFileB(const void *ptr, size_t sizeInByte);

void writeEmptyAlignment(uint32_t alignment);

extern void closeFile();

#endif //PCC_CC_FILE_H
