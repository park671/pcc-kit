//
// Created by Park Yu on 2024/10/10.
//

#ifndef MICRO_CC_MSPACE_H
#define MICRO_CC_MSPACE_H

#include <stdlib.h>

extern void *pccMalloc(const char *spaceTag, size_t size);

extern void pccFree(const char *spaceTag, void *pointer);

extern void pccFreeSpace(const char *spaceTag);

#endif //MICRO_CC_MSPACE_H
