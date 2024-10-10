//
// Created by Park Yu on 2024/9/23.
//

#ifndef MICRO_CC_ASM_ARM64_H
#define MICRO_CC_ASM_ARM64_H

#include "../compiler/mir.h"

//arm64 need 4byte alignment
#define PAGE_ALIGNMENT 2

enum SectionType {
    SECTION_TEXT,
    SECTION_DATA,
    SECTION_BSS,
    SECTION_RODATA,
};

struct StackVar {
    const char *varName;
    int varSize;
    int stackOffset;
};

void generateArm64Asm(Mir *mir, const char *assemblyFileName);

#endif //MICRO_CC_ASM_ARM64_H
