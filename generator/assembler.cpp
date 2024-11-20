//
// Created by Park Yu on 2024/10/11.
//
#include "assembler.h"
#include "logger.h"
#include "arm64/assembler_arm64.h"
#include "arm64/binary_arm64.h"
#include "elf.h"
#include "file.h"
#include "pe.h"

static const char *ASSEMBLER_TAG = "assembler";

void generateElfArm64(Mir *mir,
                      int sharedLibrary,
                      const char *outputFileName) {
    if (outputFileName == nullptr) {
        outputFileName = "output.elf";
    }
    openFile(outputFileName);

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
                                                            SHF_STRINGS | SHF_ALLOC,
                                                            currentOffset,
                                                            currentOffset,
                                                            52,
                                                            0,
                                                            0,
                                                            4,
                                                            0);

    int programHeaderOffset = sizeof(Elf64_Ehdr);
    int sectionHeaderOffset = programHeaderOffset + (sizeof(Elf64_Phdr) * programHeaderCount);


    Elf64_Ehdr *elfHeader = createElfHeader(ET_DYN,
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
    logd(ASSEMBLER_TAG, "elf arm64 generation finish.");
}

void generatePeArm64(Mir *mir,
                     int sharedLibrary,
                     const char *outputFileName) {
    if (outputFileName == nullptr) {
        outputFileName = "output.exe";
    }
    openFile(outputFileName);

    const int sectionAlignment = 4096; // 内存对齐
    const int fileAlignment = 512;    // 文件对齐

    int currentOffset = 0;
    // 1. 生成指令缓冲区
    int sectionCount = generateArm64Target(mir); // 根据 mir 生成指令
    relocateBinary(0);
    InstBuffer *instBuffer = getEmittedInstBuffer(); // 获取生成的指令缓冲区
    // 2. PE 可选头与节偏移设置
    currentOffset += getPeHeaderSize();
    //pe header(dos + coff + optional) + section headers(x1)
    //current offset is at 1st real section
    currentOffset += sectionCount * sizeof(SectionHeader);
    if (sizeof(SectionHeader) != 40) {
        loge(ASSEMBLER_TAG, "wtf: SectionHeader is not 40byte?");
        exit(-1);
    }

    int textSectionFileOffset = alignTo(currentOffset, fileAlignment);
    int textSectionFileSize = alignTo(instBuffer->size, fileAlignment);
    int textSectionVirtualAddress = alignTo(currentOffset, sectionAlignment);
    int textSectionVirtualSize = alignTo(instBuffer->size, sectionAlignment);

    // 3. 生成节表
    SectionHeader *textSectionHeader = createSectionHeader(".text",
                                                           textSectionVirtualSize,
                                                           textSectionVirtualAddress,
                                                           textSectionFileSize,
                                                           textSectionFileOffset,
                                                           IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                                           IMAGE_SCN_CNT_CODE); // 可执行，可读

    // 4. 生成 PE 头部
    void *peHeader = createPeHeader(IMAGE_FILE_MACHINE_ARM64, // ARM64 架构
                                    sectionCount,
                                    textSectionFileSize,
                                    textSectionVirtualAddress,
                                    IMAGE_BASE_64_EXE, // 默认 ImageBase
                                    sectionAlignment,
                                    fileAlignment);

    // 5. 写入 PE 文件
    writeFileB(peHeader, getPeHeaderSize());//write header
    writeFileB(textSectionHeader, sizeof(SectionHeader));
    writeEmptyAlignment(fileAlignment);
    writeFileB(instBuffer->result, instBuffer->size); // 写入指令
    writeEmptyAlignment(fileAlignment);
    logd(ASSEMBLER_TAG, "pe arm64 generation finish.");
}

void generateTargetFile(
        Mir *mir,
        Arch arch,
        Platform platform,
        int sharedLibrary,
        const char *outputFileName) {
    logd(ASSEMBLER_TAG, "target generation...");
    if (platform == PLATFORM_LINUX) {
        switch (arch) {
            case ARCH_ARM64: {
                generateElfArm64(mir, sharedLibrary, outputFileName);
                break;
            }
            case ARCH_X86_64: {
                loge(ASSEMBLER_TAG, "not impl platform yet!");
                break;
            }
            default: {

            }
        }
    } else if (platform == PLATFORM_WINDOWS) {
        switch (arch) {
            case ARCH_ARM64: {
                generatePeArm64(mir, sharedLibrary, outputFileName);
                break;
            }
            case ARCH_X86_64: {
                loge(ASSEMBLER_TAG, "not impl platform yet!");
                break;
            }
            default: {
                loge(ASSEMBLER_TAG, "unsupported architecture!");
            }
        }
    } else {
        loge(ASSEMBLER_TAG, "not impl platform yet!");
    }
    closeFile();
}