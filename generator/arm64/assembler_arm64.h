//
// Created by Park Yu on 2024/9/23.
//

#ifndef PCC_CC_ASM_ARM64_H
#define PCC_CC_ASM_ARM64_H

#include "mir.h"
#include "assembler.h"

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
    uint64_t stackOffset;
};

/**
 *
 * @param mir
 * @param outputAssembly
 * @param platform
 * @param outputFileName
 * @return section count
 */
int generateArm64Target(Mir *mir);

#endif //PCC_CC_ASM_ARM64_H
