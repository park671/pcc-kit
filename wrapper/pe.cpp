//
// Created by Park Yu on 2024/10/10.
//

#include "pe.h"

// 创建并填充 PE 头部
void *createPEHeader(uint16_t machine,
                     uint16_t numberOfSections,
                     uint32_t sizeOfCode,
                     uint32_t entryPoint,
                     uint64_t imageBase,
                     uint32_t sectionAlignment,
                     uint32_t fileAlignment) {
    // 分配 DOS 头内存
    DOSHeader *dosHeader = (DOSHeader *) malloc(sizeof(DOSHeader));
    if (dosHeader == NULL) {
        printf("Memory allocation for DOS header failed\n");
        return NULL;
    }

    // 填充 DOS 头
    memset(dosHeader, 0, sizeof(DOSHeader));
    dosHeader->e_magic = 0x5A4D;  // "MZ"
    dosHeader->e_lfanew = sizeof(DOSHeader);  // PE 头偏移

    // 分配 COFF 头内存
    COFFHeader *coffHeader = (COFFHeader *) malloc(sizeof(COFFHeader));
    if (coffHeader == NULL) {
        printf("Memory allocation for COFF header failed\n");
        free(dosHeader);
        return NULL;
    }

    // 填充 COFF 头
    memset(coffHeader, 0, sizeof(COFFHeader));
    coffHeader->Signature = 0x00004550;  // "PE\0\0"
    coffHeader->Machine = machine;
    coffHeader->NumberOfSections = numberOfSections;
    coffHeader->SizeOfOptionalHeader = sizeof(OptionalHeader);
    coffHeader->Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;  // 可执行、无重定位信息

    // 分配可选头内存
    OptionalHeader *optionalHeader = (OptionalHeader *) malloc(sizeof(OptionalHeader));
    if (optionalHeader == NULL) {
        printf("Memory allocation for Optional header failed\n");
        free(dosHeader);
        free(coffHeader);
        return NULL;
    }

    // 填充可选头
    memset(optionalHeader, 0, sizeof(OptionalHeader));
    optionalHeader->Magic = 0x20B;  // PE32+
    optionalHeader->SizeOfCode = sizeOfCode;
    optionalHeader->AddressOfEntryPoint = entryPoint;
    optionalHeader->ImageBase = imageBase;
    optionalHeader->SectionAlignment = sectionAlignment;
    optionalHeader->FileAlignment = fileAlignment;
    optionalHeader->SizeOfImage = sectionAlignment * (numberOfSections + 1);
    optionalHeader->SizeOfHeaders = sizeof(DOSHeader) + sizeof(COFFHeader) + sizeof(OptionalHeader);

    // 返回复合头部（用户可以自行整合到文件中）
    void *peHeader = malloc(sizeof(DOSHeader) + sizeof(COFFHeader) + sizeof(OptionalHeader));
    memcpy(peHeader, dosHeader, sizeof(DOSHeader));
    memcpy((char *) peHeader + sizeof(DOSHeader), coffHeader, sizeof(COFFHeader));
    memcpy((char *) peHeader + sizeof(DOSHeader) + sizeof(COFFHeader), optionalHeader, sizeof(OptionalHeader));

    // 释放临时分配的头部内存
    free(dosHeader);
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
    SectionHeader *shdr = (SectionHeader *) malloc(sizeof(SectionHeader));

    // 检查分配是否成功
    if (shdr == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    // 填充节头字段
    memset(shdr->Name, 0, sizeof(shdr->Name));
    strncpy(shdr->Name, name, 8); // 设置节名称（最多8字节）
    shdr->VirtualSize = virtualSize;          // 设置节的虚拟大小
    shdr->VirtualAddress = virtualAddress;    // 设置节的虚拟地址
    shdr->SizeOfRawData = sizeOfRawData;      // 设置节在文件中的大小
    shdr->PointerToRawData = pointerToRawData;// 设置节在文件中的偏移
    shdr->PointerToRelocations = 0;          // 无重定位
    shdr->PointerToLinenumbers = 0;          // 无行号表
    shdr->NumberOfRelocations = 0;           // 重定位表条目数
    shdr->NumberOfLinenumbers = 0;           // 行号表条目数
    shdr->Characteristics = characteristics; // 设置节的标志

    return shdr; // 返回已填充的节头指针
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
