//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_CC_MSPACE_H
#define PCC_CC_MSPACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

extern void *pccMalloc(const char *spaceTag, size_t size);

extern void pccFree(const char *spaceTag, void *pointer);

extern void pccFreeSpace(const char *spaceTag);

#ifdef __cplusplus
}
#endif

#endif //PCC_CC_MSPACE_H
