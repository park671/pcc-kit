//
// Created by Park Yu on 2024/10/10.
//

#include "pe.h"
#include "logger.h"
#include "pe_dos_header.h"

static const char *PE_TAG = "pe";

size_t getPeHeaderSize() {
    return DOS_HEADER_LENGTH + sizeof(CoffHeader) + sizeof(OptionalHeader);
}

// 创建并填充 PE 头部
CoffHeader *createCoffHeader(uint16_t machine,
                             uint16_t numberOfSections) {
    // 分配 COFF 头内存
    CoffHeader *coffHeader = (CoffHeader *) malloc(sizeof(CoffHeader));
    if (coffHeader == nullptr) {
        loge(PE_TAG, "Memory allocation for COFF header failed\n");
        return nullptr;
    }
    // 填充 COFF 头
    memset(coffHeader, 0, sizeof(CoffHeader));
    coffHeader->signature = 0x00004550;  // "PE\0\0"
    coffHeader->machine = machine;
    coffHeader->numberOfSections = numberOfSections;
    coffHeader->sizeOfOptionalHeader = sizeof(OptionalHeader);
    coffHeader->characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;  // 可执行、无重定位信息

    return coffHeader;


}

OptionalHeader *createOptionalHeader(uint16_t numberOfSections,
                                     uint32_t sizeOfCode,
                                     uint32_t entryPoint,
                                     uint64_t imageBase,
                                     uint32_t sectionAlignment,
                                     uint32_t fileAlignment) {
    // 分配可选头内存
    OptionalHeader *optionalHeader = (OptionalHeader *) malloc(sizeof(OptionalHeader));
    if (optionalHeader == nullptr) {
        loge(PE_TAG, "Memory allocation for Optional header failed\n");
        return nullptr;
    }

    // 填充可选头
    memset(optionalHeader, 0, sizeof(OptionalHeader));

    // 基础字段
    optionalHeader->magic = 0x20B; // PE32+ magic number
    optionalHeader->majorLinkerVersion = 14; // Linker major version
    optionalHeader->minorLinkerVersion = 42; // Linker minor version
    optionalHeader->sizeOfCode = sizeOfCode; // size of code
    optionalHeader->sizeOfInitializedData = 0x6600; // size of initialized data
    optionalHeader->sizeOfUninitializedData = 0; // size of uninitialized data
    optionalHeader->addressOfEntryPoint = entryPoint; // Entry point address
    optionalHeader->baseOfCode = 0x1000; // Base of code
    optionalHeader->imageBase = imageBase; // Image base
    optionalHeader->sectionAlignment = sectionAlignment; // Section alignment
    optionalHeader->fileAlignment = fileAlignment; // File alignment

    // 操作系统、镜像和子系统信息
    optionalHeader->majorOperatingSystemVersion = 6; // OS major version
    optionalHeader->minorOperatingSystemVersion = 2; // OS minor version
    optionalHeader->majorImageVersion = 0; // Image major version
    optionalHeader->minorImageVersion = 0; // Image minor version
    optionalHeader->majorSubsystemVersion = 6; // subsystem major version
    optionalHeader->minorSubsystemVersion = 2; // subsystem minor version
    optionalHeader->win32VersionValue = 0; // Reserved, must be 0
    optionalHeader->sizeOfImage = 0x2000; // size of image fixme
    optionalHeader->sizeOfHeaders = getPeHeaderSize(); // size of headers
    optionalHeader->checkSum = 0; // Checksum (can be 0 if unused)
    optionalHeader->subsystem = 3; // subsystem (3 = Windows CUI)
    optionalHeader->dllCharacteristics = 0x8160; // DLL characteristics:
    // High Entropy Virtual Addresses, Dynamic base, NX compatible, Terminal Server Aware

    // 堆栈和堆的大小
    optionalHeader->sizeOfStackReserve = 0x100000; // Stack reserve size
    optionalHeader->sizeOfStackCommit = 0x1000; // Stack commit size
    optionalHeader->sizeOfHeapReserve = 0x100000; // Heap reserve size
    optionalHeader->sizeOfHeapCommit = 0x1000; // Heap commit size
    optionalHeader->loaderFlags = 0; // Loader flags (reserved, must be 0)
    optionalHeader->numberOfRvaAndSizes = 16; // Number of directories

    // 数据目录 (Data Directory)
    uint32_t dataDirectories[16][2] = {
            {0x00000000, 0x00000000}, // Export Directory
            {0x00021348, 0x00000050}, // Import Directory
            {0x00024000, 0x0000043C}, // Resource Directory
            {0x0001F000, 0x00001368}, // Exception Directory
            {0x00000000, 0x00000000}, // Certificates Directory
            {0x00025000, 0x00000058}, // Base Relocation Directory
            {0x0001C780, 0x00000038}, // Debug Directory
            {0x00000000, 0x00000000}, // Architecture Directory
            {0x00000000, 0x00000000}, // Global Pointer Directory
            {0x00000000, 0x00000000}, // Thread Storage Directory
            {0x0001C600, 0x00000140}, // Load Configuration Directory
            {0x00000000, 0x00000000}, // Bound Import Directory
            {0x00021000, 0x00000348}, // Import Address Table Directory
            {0x00000000, 0x00000000}, // Delay Import Directory
            {0x00000000, 0x00000000}, // COM Descriptor Directory
            {0x00000000, 0x00000000}  // Reserved Directory
    };

    // 将数据目录逐一填充到可选头中
    for (int i = 0; i < 16; i++) {
//        optionalHeader->dataDirectory[i].virtualAddress = dataDirectories[i][0];
//        optionalHeader->dataDirectory[i].size = dataDirectories[i][1];
        optionalHeader->dataDirectory[i].virtualAddress = 0;
        optionalHeader->dataDirectory[i].size = 0;
    }

    return optionalHeader;
}


void *createPeHeader(uint16_t machine,
                     uint16_t numberOfSections,
                     uint32_t sizeOfCode,
                     uint32_t entryPoint,
                     uint64_t imageBase,
                     uint32_t sectionAlignment,
                     uint32_t fileAlignment) {
    CoffHeader *coffHeader = createCoffHeader(machine, numberOfSections);
    OptionalHeader *optionalHeader = createOptionalHeader(numberOfSections,
                                                          sizeOfCode,
                                                          entryPoint,
                                                          imageBase,
                                                          sectionAlignment,
                                                          fileAlignment);
    // 返回复合头部（用户可以自行整合到文件中）
    void *peHeader = malloc(getPeHeaderSize());
    memcpy(peHeader, DOS_HEADER, DOS_HEADER_LENGTH);
    memcpy((char *) peHeader + DOS_HEADER_LENGTH, coffHeader, sizeof(CoffHeader));
    memcpy((char *) peHeader + DOS_HEADER_LENGTH + sizeof(CoffHeader), optionalHeader, sizeof(OptionalHeader));
    free(coffHeader);
    free(optionalHeader);
    return peHeader;
}

// 创建并填充节表的方法
SectionHeader *createSectionHeader(const char *name,
                                   uint32_t virtualSize,
                                   uint32_t virtualAddress,
                                   uint32_t sizeOfRawData,
                                   uint32_t pointerToRawData,
                                   uint32_t characteristics) {
    // 分配节头的内存
    SectionHeader *sectionheader = (SectionHeader *) malloc(sizeof(SectionHeader));

    // 检查分配是否成功
    if (sectionheader == nullptr) {
        loge(PE_TAG, "Error: memory allocation failed\n");
        return nullptr;
    }

    // 填充节头字段
    memset(sectionheader->name, 0, sizeof(sectionheader->name));
    strncpy(sectionheader->name, name, 8); // 设置节名称（最多8字节）
    sectionheader->virtualSize = virtualSize;          // 设置节的虚拟大小
    sectionheader->virtualAddress = virtualAddress;    // 设置节的虚拟地址
    sectionheader->sizeOfRawData = sizeOfRawData;      // 设置节在文件中的大小
    sectionheader->pointerToRawData = pointerToRawData;// 设置节在文件中的偏移
    sectionheader->pointerToRelocations = 0;          // 无重定位
    sectionheader->pointerToLineNumbers = 0;          // 无行号表
    sectionheader->numberOfRelocations = 0;           // 重定位表条目数
    sectionheader->numberOfLineNumbers = 0;           // 行号表条目数
    sectionheader->characteristics = characteristics; // 设置节的标志

    return sectionheader; // 返回已填充的节头指针
}

// 创建并填充程序段的方法
ProgramSegment *createProgramSegment(uint32_t type,
                                     uint32_t flags,
                                     uint32_t fileOffset,
                                     uint32_t virtualAddress,
                                     uint32_t fileSize,
                                     uint32_t memorySize,
                                     uint32_t alignment) {
    // 分配段信息的内存
    ProgramSegment *segment = (ProgramSegment *) malloc(sizeof(ProgramSegment));

    // 检查分配是否成功
    if (segment == nullptr) {
        loge(PE_TAG, "Error: memory allocation failed\n");
        return nullptr;
    }

    // 填充段信息字段
    segment->type = type;              // 设置段类型
    segment->flags = flags;            // 设置段标志
    segment->fileOffset = fileOffset;  // 设置段在文件中的偏移
    segment->virtualAddress = virtualAddress; // 设置段的虚拟地址
    segment->fileSize = fileSize;      // 设置段在文件中的大小
    segment->memorySize = memorySize;  // 设置段在内存中的大小
    segment->alignment = alignment;    // 设置段的对齐要求

    return segment; // 返回已填充的段信息指针
}
