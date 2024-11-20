//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_PE_H
#define PCC_PE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <stdint.h>

// 定义 PE 文件头中的 Machine 字段枚举
enum {
    IMAGE_FILE_MACHINE_UNKNOWN     = 0x0,      // 未知架构
    IMAGE_FILE_MACHINE_AMD64       = 0x8664,   // x64 (AMD64/EM64T)
    IMAGE_FILE_MACHINE_ARM         = 0x1c0,    // ARM 小端
    IMAGE_FILE_MACHINE_ARM64       = 0xaa64,   // ARM64 (AArch64)
    IMAGE_FILE_MACHINE_I386        = 0x14c,    // Intel 386
    IMAGE_FILE_MACHINE_RISCV32     = 0x5032,   // RISC-V 32 位
    IMAGE_FILE_MACHINE_RISCV64     = 0x5064,   // RISC-V 64 位
    IMAGE_FILE_MACHINE_RISCV128    = 0x5128,   // RISC-V 128 位
} MachineType;

enum {
    IMAGE_BASE_32_EXE = 0x00400000,
    IMAGE_BASE_64_EXE = 0x0000000140000000,
    IMAGE_BASE_32_DLL = 0x10000000,
    IMAGE_BASE_64_DLL = 0x0000000180000000,
} ImageBaseAddr;

#include <stdint.h>

// 定义 Characteristics 枚举
enum {
    IMAGE_FILE_RELOCS_STRIPPED         = 0x0001, // 没有重定位信息
    IMAGE_FILE_EXECUTABLE_IMAGE        = 0x0002, // 文件是可执行的
    IMAGE_FILE_LINE_NUMS_STRIPPED      = 0x0004, // 没有行号信息
    IMAGE_FILE_LOCAL_SYMS_STRIPPED     = 0x0008, // 没有本地符号信息
    IMAGE_FILE_AGGRESSIVE_WS_TRIM      = 0x0010, // 已废弃
    IMAGE_FILE_LARGE_ADDRESS_AWARE     = 0x0020, // 应用可以使用大于 2GB 的地址空间
    IMAGE_FILE_BYTES_REVERSED_LO       = 0x0080, // 小端字节序（已废弃）
    IMAGE_FILE_32BIT_MACHINE           = 0x0100, // 文件为 32 位格式
    IMAGE_FILE_DEBUG_STRIPPED          = 0x0200, // 已剥离调试信息
    IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400, // 从可移动设备运行时，优先加载到交换空间
    IMAGE_FILE_NET_RUN_FROM_SWAP       = 0x0800, // 从网络运行时，优先加载到交换空间
    IMAGE_FILE_SYSTEM                  = 0x1000, // 系统文件（如驱动程序）
    IMAGE_FILE_DLL                     = 0x2000, // 文件是动态链接库（DLL）
    IMAGE_FILE_UP_SYSTEM_ONLY          = 0x4000, // 仅支持单处理器系统
    IMAGE_FILE_BYTES_REVERSED_HI       = 0x8000  // 大端字节序（已废弃）
} PECharacteristics;

// 定义 DOS 头结构
typedef struct {
    uint16_t e_magic;    // DOS 魔术字节 (0x5A4D, "MZ")
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;   // PE 头的偏移地址
} DOSHeader;

// 定义 COFF 头结构
typedef struct {
    uint32_t Signature;   // "PE\0\0" 签名
    uint16_t Machine;     // 目标架构 (0x8664 for x64)
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} COFFHeader;

// 定义可选头（仅支持 PE32+ 格式）
typedef struct {
    uint16_t Magic;               // 0x20B 表示 PE32+
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} OptionalHeader;

// 定义数据目录（如果需要）
typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} DataDirectory;


// 定义节表结构
typedef struct {
    char Name[8];                // 节名称
    uint32_t VirtualSize;        // 节的虚拟大小
    uint32_t VirtualAddress;     // 节的虚拟地址
    uint32_t SizeOfRawData;      // 节在文件中的大小
    uint32_t PointerToRawData;   // 节在文件中的偏移
    uint32_t PointerToRelocations; // 重定位表的偏移（通常为 0）
    uint32_t PointerToLinenumbers; // 行号表偏移（通常为 0）
    uint16_t NumberOfRelocations; // 重定位表条目数
    uint16_t NumberOfLinenumbers; // 行号表条目数
    uint32_t Characteristics;    // 节的标志（如可读、可执行）
} SectionHeader;


// 定义段信息结构（自定义，用于模拟段的描述）
typedef struct {
    uint32_t Type;               // 段类型（模拟 ELF 的段类型）
    uint32_t Flags;              // 段标志（可读、可写、可执行）
    uint32_t FileOffset;         // 段在文件中的偏移
    uint32_t VirtualAddress;     // 段的虚拟地址
    uint32_t FileSize;           // 段在文件中的大小
    uint32_t MemorySize;         // 段在内存中的大小
    uint32_t Alignment;          // 对齐方式
} ProgramSegment;

// 创建并填充 PE 头部
void *createPEHeader(uint16_t machine,
                     uint16_t numberOfSections,
                     uint32_t sizeOfCode,
                     uint32_t entryPoint,
                     uint64_t imageBase,
                     uint32_t sectionAlignment,
                     uint32_t fileAlignment);

// 创建并填充节表的方法
SectionHeader *createSectionHeader(const char *name,
                                   uint32_t virtualSize,
                                   uint32_t virtualAddress,
                                   uint32_t sizeOfRawData,
                                   uint32_t pointerToRawData,
                                   uint32_t characteristics);

// 创建并填充程序段的方法
ProgramSegment *createProgramSegment(uint32_t type,
                                     uint32_t flags,
                                     uint32_t fileOffset,
                                     uint32_t virtualAddress,
                                     uint32_t fileSize,
                                     uint32_t memorySize,
                                     uint32_t alignment);

#endif //PCC_PE_H
