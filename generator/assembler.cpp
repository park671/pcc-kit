//
// Created by Park Yu on 2024/10/11.
//
#include "assembler.h"
#include "logger.h"
#include "arm64/assembler_arm64.h"
#include "arm64/binary_arm64.h"
#include "elf.h"
#include "file.h"

static const char *ASSEMBLER_TAG = "assembler";

void generateTargetFile(
        Mir *mir,
        Arch arch,
        Platform platform,
        int outputAssembly,
        int sharedLibrary,
        const char *outputFileName) {
    logd(ASSEMBLER_TAG, "target generation...");

    if (outputFileName == nullptr) {
        if (outputAssembly) {
            outputFileName = "output.s";
        } else {
            outputFileName = "output.bin";
        }
    }
    openFile(outputFileName);
    if (platform == PLATFORM_LINUX) {
        switch (arch) {
            case ARCH_ARM64: {
                int currentOffset = 0;
                int programHeaderCount = 0;
                int sectionHeaderCount = 1;//shstrtab

                int sectionCount = generateArm64Target(mir);
                currentOffset += sizeof(Elf64_Ehdr);
                programHeaderCount += sectionCount;
                sectionHeaderCount += sectionCount;
                currentOffset += programHeaderCount * sizeof(Elf64_Phdr);
                currentOffset += sectionHeaderCount * sizeof(Elf64_Shdr);

                int programEntry = currentOffset;

                relocateBinary(0);
                InstBuffer *instBuffer = getEmittedInstBuffer();
                Elf64_Phdr *programHeader = createProgramHeader(PT_LOAD,
                                                                PF_R | PF_X,
                                                                0,
                                                                0,
                                                                0,
                                                                currentOffset + instBuffer->size,
                                                                currentOffset + instBuffer->size,
                                                                4096);
                Elf64_Shdr *sectionHeader = createSectionHeader(TEXT_SECTION_IDX,
                                                                SHT_PROGBITS,
                                                                SHF_ALLOC | SHF_EXECINSTR,
                                                                currentOffset,
                                                                currentOffset,
                                                                instBuffer->size,
                                                                0,
                                                                0,
                                                                4,
                                                                0);
                currentOffset += instBuffer->size;

                Elf64_Shdr *shstrtabSectionHeader = createSectionHeader(SHSTRTAB_SECTION_IDX,
                                                                        SHT_STRTAB,
                                                                        SHF_STRINGS,
                                                                        currentOffset,
                                                                        currentOffset,
                                                                        52,
                                                                        0,
                                                                        0,
                                                                        4,
                                                                        0);

                int programHeaderOffset = sizeof(Elf64_Ehdr);
                int sectionHeaderOffset = programHeaderOffset + (sizeof(Elf64_Phdr) * programHeaderCount);


                Elf64_Ehdr *elfHeader = createElfHeader(ET_EXEC,
                                                        EM_AARCH64,
                                                        programEntry,
                                                        programHeaderOffset,
                                                        sectionHeaderOffset,
                                                        sizeof(Elf64_Phdr),
                                                        programHeaderCount,
                                                        sizeof(Elf64_Shdr),
                                                        sectionHeaderCount,
                                                        sectionHeaderCount - 1);
                writeFileB(elfHeader, sizeof(Elf64_Ehdr));
                for (int i = 0; i < sectionCount; i++) {
                    //todo real impl
                    writeFileB(programHeader, sizeof(Elf64_Phdr));
                }
                for (int i = 0; i < sectionCount; i++) {
                    //todo real impl
                    writeFileB(sectionHeader, sizeof(Elf64_Shdr));
                }
                writeFileB(shstrtabSectionHeader, sizeof(Elf64_Shdr));
                writeFileB(instBuffer->result, instBuffer->size);
                writeFileB(shstrtabString, shstrtabSize);
                break;
            }
            case ARCH_X86_64: {

                break;
            }
            default: {

            }
        }
    } else {
        loge(ASSEMBLER_TAG, "not impl platform yet!");
    }
    closeFile();
}