//
// Created by Park Yu on 2024/10/11.
//
#include "assembler.h"
#include "logger.h"
#include "arm64/assembler_arm64.h"
#include "arm64/binary_arm64.h"
#include "arm64/linux_syscall.h"
#include "arm64/windows_syscall.h"
#include "arm64/macos_syscall.h"
#include "elf.h"
#include "file.h"
#include "pe.h"
#include "macho.h"
#include "sys/syscall.h"

static const char *ASSEMBLER_TAG = "assembler";

void generateElfArm64(Mir *mir,
                      int sharedLibrary,
                      const char *outputFileName) {
    if (outputFileName == nullptr) {
        outputFileName = "output.elf";
    }
    openFile(outputFileName);

    uint64_t programEntry = 0;
    int programHeaderCount = 0;
    int sectionHeaderCount = 1;//shstrtab
    initLinuxArm64ProgramStart();
    int sectionCount = generateArm64Target(mir);
    programEntry += sizeof(Elf64_Ehdr);//elf header
    programHeaderCount += sectionCount;
    sectionHeaderCount += sectionCount;
    programEntry += programHeaderCount * sizeof(Elf64_Phdr);//program header
    programEntry += sectionHeaderCount * sizeof(Elf64_Shdr);//section header

    programEntry = alignTo(programEntry, 4096);
    uint64_t textBufferSize = getInstBufferSize();
    uint64_t dataEntry = alignTo(programEntry + textBufferSize, 4096);//file & vaddr use same alignment
    //relocate & get binary
    relocateBinary(dataEntry);
    InstBuffer *instBuffer = getEmittedInstBuffer();
    DataBuffer *dataBuffer = getEmittedDataBuffer();

    Elf64_Phdr *textProgramHeader = createProgramHeader(PT_LOAD,
                                                        PF_R | PF_X,
                                                        0,
                                                        0,
                                                        0,
                                                        programEntry + instBuffer->size,
                                                        programEntry + instBuffer->size,
                                                        4096);

    Elf64_Phdr *dataProgramHeader = createProgramHeader(PT_LOAD,
                                                        PF_R | PF_W,
                                                        dataEntry,
                                                        dataEntry,
                                                        dataEntry,
                                                        dataBuffer->size,
                                                        dataBuffer->size,
                                                        4096);
    Elf64_Shdr *textSectionHeader = createSectionHeader(TEXT_SECTION_IDX,
                                                        SHT_PROGBITS,
                                                        SHF_ALLOC | SHF_EXECINSTR,
                                                        programEntry,
                                                        programEntry,
                                                        instBuffer->size,
                                                        0,
                                                        0,
                                                        4,
                                                        0);

    Elf64_Shdr *dataSectionHeader = createSectionHeader(DATA_SECTION_IDX,
                                                        SHT_PROGBITS,
                                                        SHF_WRITE | SHF_ALLOC,
                                                        dataEntry,
                                                        dataEntry,
                                                        dataBuffer->size,
                                                        0,
                                                        0,
                                                        4,
                                                        0);

    uint64_t strTabEntry = alignTo(dataEntry + dataBuffer->size, 4096);

    Elf64_Shdr *shstrtabSectionHeader = createSectionHeader(SHSTRTAB_SECTION_IDX,
                                                            SHT_STRTAB,
                                                            SHF_STRINGS | SHF_ALLOC,
                                                            strTabEntry,
                                                            strTabEntry,
                                                            52,
                                                            0,
                                                            0,
                                                            4,
                                                            0);

    uint64_t programHeaderOffset = sizeof(Elf64_Ehdr);
    uint64_t sectionHeaderOffset = programHeaderOffset + (sizeof(Elf64_Phdr) * programHeaderCount);


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
    //program header
    writeFileB(textProgramHeader, sizeof(Elf64_Phdr));
    writeFileB(dataProgramHeader, sizeof(Elf64_Phdr));
    //section header
    writeFileB(textSectionHeader, sizeof(Elf64_Shdr));
    writeFileB(dataSectionHeader, sizeof(Elf64_Shdr));
    writeFileB(shstrtabSectionHeader, sizeof(Elf64_Shdr));
    //binary buffer
    writeEmptyAlignment(4096);
    writeFileB(instBuffer->result, instBuffer->size);
    writeEmptyAlignment(4096);
    writeFileB(dataBuffer->result, dataBuffer->size);
    writeEmptyAlignment(4096);
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
    initWindowsArm64ProgramStart();
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

void generateMachoArm64(Mir *mir,
                        int sharedLibrary,
                        const char *outputFileName) {
    if (outputFileName == nullptr) {
        outputFileName = "output.macho";
    }
    openFile(outputFileName);

    const uint64_t ram_alignment = 16384;
    const uint64_t file_alignment = 4;

    int loadCommandCount = 0;
    uint64_t currentFileAddr = 0;
    uint64_t currentVmAddr = 0;

    initMachOProgramStart();

    // Generate ARM64 target code
    generateArm64Target(mir);
    relocateBinary(0);
    InstBuffer *instBuffer = getEmittedInstBuffer();

    // Calculate offsets and counts
    /**
     * mach-o header
     * [
     *  segment64: page0
     *  segment64: __TEXT
     *  segment64: lc_main
     * ]
     */
    currentFileAddr += sizeof(mach_header_64);//header
    currentFileAddr += sizeof(segment_command_64);//__PAGE0
    currentFileAddr += sizeof(segment_command_64);//__TEXT
    currentFileAddr += sizeof(entry_point_command);//LC_MAIN
    currentFileAddr += sizeof(section_64);//__text section

    uint64_t codeFileAddr = alignTo(currentFileAddr, file_alignment);

    uint64_t loadCommandSize = 0;
    loadCommandSize += sizeof(segment_command_64);//__PAGE0
    loadCommandSize += sizeof(segment_command_64);//__TEXT
    loadCommandSize += sizeof(entry_point_command);//LC_MAIN
    //lc_segment64: page0
    const uint64_t page0Size = 0x100000000;
    uint64_t pageZeroSize = alignTo(page0Size, ram_alignment);
    segment_command_64 *pageZeroLc = createSegmentCommand64("__PAGEZERO",
                                                            currentVmAddr,
                                                            pageZeroSize,
                                                            0,
                                                            0,
                                                            VM_PROT_NONE,
                                                            VM_PROT_NONE,
                                                            0,
                                                            0);
    loadCommandCount++;
    currentVmAddr += pageZeroSize;
    currentVmAddr = alignTo(currentVmAddr, ram_alignment);

    //lc_segment64: text
    uint64_t textVmSize = alignTo(codeFileAddr + instBuffer->size, ram_alignment);
    const char *TEXT_SEGMENT_NAME = "__TEXT";
    segment_command_64 *textLc = createSegmentCommand64(TEXT_SEGMENT_NAME,
                                                        currentVmAddr,
                                                        textVmSize,
                                                        0,
                                                        textVmSize,
                                                        VM_PROT_EXECUTE | VM_PROT_READ,
                                                        VM_PROT_EXECUTE | VM_PROT_READ,
                                                        1,
                                                        0);
    loadCommandCount++;
    currentVmAddr += textVmSize;
    currentVmAddr = alignTo(currentVmAddr, ram_alignment);

    //lc_main
    entry_point_command *mainLc = createEntryPointCommand(codeFileAddr, 0);
    loadCommandCount++;

    //section64:text
    uint64_t codeVmAddr = page0Size + codeFileAddr;
    section_64 *textSection = createSection64("__text",
                                              TEXT_SEGMENT_NAME,
                                              codeVmAddr,
                                              instBuffer->size,
                                              codeFileAddr,
                                              file_alignment,
                                              0,
                                              0,
                                              S_REGULAR | S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS);

    mach_header_64 *machHeader = createMachHeader64(
            CPU_TYPE_ARM64,
            CPU_SUBTYPE_ARM64_ALL,
            MH_EXECUTE,
            loadCommandCount,
            loadCommandSize,
            MH_NOUNDEFS | MH_PIE);

    writeFileB(machHeader, sizeof(mach_header_64));
    writeFileB(pageZeroLc, sizeof(segment_command_64));
    writeFileB(textLc, sizeof(segment_command_64));
    writeFileB(mainLc, sizeof(entry_point_command));
    writeFileB(textSection, sizeof(section_64));
    writeEmptyAlignment(file_alignment);
    writeFileB(instBuffer->result, instBuffer->size);
    logd(ASSEMBLER_TAG, "mach-o arm64 generation finish.");
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
    } else if (platform == PLATFORM_MACOS) {
        switch (arch) {
            case ARCH_ARM64: {
                generateMachoArm64(mir, sharedLibrary, outputFileName);
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