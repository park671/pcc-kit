//
// Created by Park Yu on 2024/10/11.
//
#include "assembler.h"
#include "logger.h"
#include "assembler_arm64.h"

static const char *ASSEMBLER_TAG = "assembler";

void generateTargetFile(
        Mir *mir,
        Arch arch,
        Platform platform,
        int outputAssembly,
        const char *outputFileName) {
    logd(ASSEMBLER_TAG, "target generation...");

    if (outputFileName == nullptr) {
        if (outputAssembly) {
            outputFileName = "output.s";
        } else {
            outputFileName = "output.bin";
        }
    }
    switch (arch) {
        case ARCH_ARM64: {
            generateArm64Target(mir, outputAssembly, outputFileName);
            break;
        }
        case ARCH_X86_64: {

            break;
        }
        default: {

        }
    }
}