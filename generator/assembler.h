//
// Created by Park Yu on 2024/10/11.
//

#ifndef PCC_ASSEMBLER_H
#define PCC_ASSEMBLER_H

#include "mir.h"

enum Arch {
    ARCH_UNKNOWN,
    ARCH_X86_64,
    ARCH_ARM64,
};

enum Platform {
    PLATFORM_UNKNOWN,
    PLATFORM_BARE,
    PLATFORM_LINUX,
    PLATFORM_MACOS,
    PLATFORM_WINDOWS,
};

/**
 *
 * @param mir
 * @param arch
 * @param platform
 * @param assemblyFileName null will generate binary file
 */
extern void generateTargetFile(
        Mir *mir,
        Arch arch,
        Platform platform,
        int assembly,
        const char *targetFileName);

#endif //PCC_ASSEMBLER_H
