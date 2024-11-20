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
    coffHeader->Signature = 0x00004550;  // "PE\0\0"
    coffHeader->Machine = machine;
    coffHeader->NumberOfSections = numberOfSections;
    coffHeader->SizeOfOptionalHeader = sizeof(OptionalHeader);
    coffHeader->Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;  // 可执行、无重定位信息

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
    if (optionalHeader == NULL) {
        loge(PE_TAG, "Memory allocation for Optional header failed\n");
        return NULL;
    }

    // 填充可选头
    memset(optionalHeader, 0, sizeof(OptionalHeader));

    // 基础字段
    optionalHeader->Magic = 0x20B; // PE32+ Magic number
    optionalHeader->MajorLinkerVersion = 14; // Linker major version
    optionalHeader->MinorLinkerVersion = 42; // Linker minor version
    optionalHeader->SizeOfCode = sizeOfCode; // Size of code
    optionalHeader->SizeOfInitializedData = 0x6600; // Size of initialized data
    optionalHeader->SizeOfUninitializedData = 0; // Size of uninitialized data
    optionalHeader->AddressOfEntryPoint = entryPoint; // Entry point address
    optionalHeader->BaseOfCode = 0x1000; // Base of code
    optionalHeader->ImageBase = imageBase; // Image base
    optionalHeader->SectionAlignment = sectionAlignment; // Section alignment
    optionalHeader->FileAlignment = fileAlignment; // File alignment

    // 操作系统、镜像和子系统信息
    optionalHeader->MajorOperatingSystemVersion = 6; // OS major version
    optionalHeader->MinorOperatingSystemVersion = 2; // OS minor version
    optionalHeader->MajorImageVersion = 0; // Image major version
    optionalHeader->MinorImageVersion = 0; // Image minor version
    optionalHeader->MajorSubsystemVersion = 6; // Subsystem major version
    optionalHeader->MinorSubsystemVersion = 2; // Subsystem minor version
    optionalHeader->Win32VersionValue = 0; // Reserved, must be 0
    optionalHeader->SizeOfImage = 0x2000; // Size of image fixme
    optionalHeader->SizeOfHeaders = getPeHeaderSize(); // Size of headers
    optionalHeader->CheckSum = 0; // Checksum (can be 0 if unused)
    optionalHeader->Subsystem = 3; // Subsystem (3 = Windows CUI)
    optionalHeader->DllCharacteristics = 0x8160; // DLL characteristics:
    // High Entropy Virtual Addresses, Dynamic base, NX compatible, Terminal Server Aware

    // 堆栈和堆的大小
    optionalHeader->SizeOfStackReserve = 0x100000; // Stack reserve size
    optionalHeader->SizeOfStackCommit = 0x1000; // Stack commit size
    optionalHeader->SizeOfHeapReserve = 0x100000; // Heap reserve size
    optionalHeader->SizeOfHeapCommit = 0x1000; // Heap commit size
    optionalHeader->LoaderFlags = 0; // Loader flags (reserved, must be 0)
    optionalHeader->NumberOfRvaAndSizes = 16; // Number of directories

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
//        optionalHeader->DataDirectory[i].VirtualAddress = dataDirectories[i][0];
//        optionalHeader->DataDirectory[i].Size = dataDirectories[i][1];
        optionalHeader->DataDirectory[i].VirtualAddress = 0;
        optionalHeader->DataDirectory[i].Size = 0;
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
    if (sectionheader == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    // 填充节头字段
    memset(sectionheader->Name, 0, sizeof(sectionheader->Name));
    strncpy(sectionheader->Name, name, 8); // 设置节名称（最多8字节）
    sectionheader->VirtualSize = virtualSize;          // 设置节的虚拟大小
    sectionheader->VirtualAddress = virtualAddress;    // 设置节的虚拟地址
    sectionheader->SizeOfRawData = sizeOfRawData;      // 设置节在文件中的大小
    sectionheader->PointerToRawData = pointerToRawData;// 设置节在文件中的偏移
    sectionheader->PointerToRelocations = 0;          // 无重定位
    sectionheader->PointerToLinenumbers = 0;          // 无行号表
    sectionheader->NumberOfRelocations = 0;           // 重定位表条目数
    sectionheader->NumberOfLinenumbers = 0;           // 行号表条目数
    sectionheader->Characteristics = characteristics; // 设置节的标志

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
    if (segment == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    // 填充段信息字段
    segment->Type = type;              // 设置段类型
    segment->Flags = flags;            // 设置段标志
    segment->FileOffset = fileOffset;  // 设置段在文件中的偏移
    segment->VirtualAddress = virtualAddress; // 设置段的虚拟地址
    segment->FileSize = fileSize;      // 设置段在文件中的大小
    segment->MemorySize = memorySize;  // 设置段在内存中的大小
    segment->Alignment = alignment;    // 设置段的对齐要求

    return segment; // 返回已填充的段信息指针
}
