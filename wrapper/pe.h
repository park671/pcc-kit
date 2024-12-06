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

// 定义 PE 文件头中的 machine 字段枚举
enum {
    IMAGE_FILE_MACHINE_UNKNOWN = 0x0,      // 未知架构
    IMAGE_FILE_MACHINE_AMD64 = 0x8664,   // x64 (AMD64/EM64T)
    IMAGE_FILE_MACHINE_ARM = 0x1c0,    // ARM 小端
    IMAGE_FILE_MACHINE_ARM64 = 0xaa64,   // ARM64 (AArch64)
    IMAGE_FILE_MACHINE_I386 = 0x14c,    // Intel 386
    IMAGE_FILE_MACHINE_RISCV32 = 0x5032,   // RISC-V 32 位
    IMAGE_FILE_MACHINE_RISCV64 = 0x5064,   // RISC-V 64 位
    IMAGE_FILE_MACHINE_RISCV128 = 0x5128,   // RISC-V 128 位
} MachineType;

enum {
    IMAGE_BASE_32_EXE = 0x00400000,
    IMAGE_BASE_64_EXE = 0x0000000140000000,
    IMAGE_BASE_32_DLL = 0x10000000,
    IMAGE_BASE_64_DLL = 0x0000000180000000,
} ImageBaseAddr;

#include <stdint.h>

// 定义 characteristics 枚举
enum {
    IMAGE_FILE_RELOCS_STRIPPED = 0x0001, // 没有重定位信息
    IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002, // 文件是可执行的
    IMAGE_FILE_LINE_NUMS_STRIPPED = 0x0004, // 没有行号信息
    IMAGE_FILE_LOCAL_SYMS_STRIPPED = 0x0008, // 没有本地符号信息
    IMAGE_FILE_AGGRESSIVE_WS_TRIM = 0x0010, // 已废弃
    IMAGE_FILE_LARGE_ADDRESS_AWARE = 0x0020, // 应用可以使用大于 2GB 的地址空间
    IMAGE_FILE_BYTES_REVERSED_LO = 0x0080, // 小端字节序（已废弃）
    IMAGE_FILE_32BIT_MACHINE = 0x0100, // 文件为 32 位格式
    IMAGE_FILE_DEBUG_STRIPPED = 0x0200, // 已剥离调试信息
    IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400, // 从可移动设备运行时，优先加载到交换空间
    IMAGE_FILE_NET_RUN_FROM_SWAP = 0x0800, // 从网络运行时，优先加载到交换空间
    IMAGE_FILE_SYSTEM = 0x1000, // 系统文件（如驱动程序）
    IMAGE_FILE_DLL = 0x2000, // 文件是动态链接库（DLL）
    IMAGE_FILE_UP_SYSTEM_ONLY = 0x4000, // 仅支持单处理器系统
    IMAGE_FILE_BYTES_REVERSED_HI = 0x8000  // 大端字节序（已废弃）
} PECharacteristics;

// 定义 COFF 头结构
typedef struct {
    uint32_t signature;   // "PE\0\0" 签名
    uint16_t machine;     // 目标架构 (0x8664 for x64)
    uint16_t numberOfSections;
    uint32_t timeDateStamp;
    uint32_t pointerToSymbolTable;
    uint32_t numberOfSymbols;
    uint16_t sizeOfOptionalHeader;
    uint16_t characteristics;
} CoffHeader;

// 定义数据目录结构
typedef struct {
    uint32_t virtualAddress; // 数据目录的 RVA
    uint32_t size;           // 数据目录的大小
} DataDirectory;

// 修正后的 OptionalHeader 结构体（支持 PE32+）
typedef struct {
    uint16_t magic;               // 0x20B 表示 PE32+
    uint8_t majorLinkerVersion;
    uint8_t minorLinkerVersion;
    uint32_t sizeOfCode;
    uint32_t sizeOfInitializedData;
    uint32_t sizeOfUninitializedData;
    uint32_t addressOfEntryPoint;
    uint32_t baseOfCode;
    uint64_t imageBase;           // 8字节（PE32+）
    uint32_t sectionAlignment;
    uint32_t fileAlignment;
    uint16_t majorOperatingSystemVersion;
    uint16_t minorOperatingSystemVersion;
    uint16_t majorImageVersion;
    uint16_t minorImageVersion;
    uint16_t majorSubsystemVersion;
    uint16_t minorSubsystemVersion;
    uint32_t win32VersionValue;   // 保留，必须为 0
    uint32_t sizeOfImage;
    uint32_t sizeOfHeaders;
    uint32_t checkSum;
    uint16_t subsystem;
    uint16_t dllCharacteristics;
    uint64_t sizeOfStackReserve;  // 栈保留大小
    uint64_t sizeOfStackCommit;   // 栈提交大小
    uint64_t sizeOfHeapReserve;   // 堆保留大小
    uint64_t sizeOfHeapCommit;    // 堆提交大小
    uint32_t loaderFlags;         // 保留，必须为 0
    uint32_t numberOfRvaAndSizes; // 数据目录数量（通常为 16）
    DataDirectory dataDirectory[16]; // 数据目录数组
} OptionalHeader;

#include <stdint.h>

// 定义 SectionHeader 的 characteristics 枚举
typedef enum {
    // 保留字段
    IMAGE_SCN_RESERVED_00000001 = 0x00000001,  // 保留，必须为 0
    IMAGE_SCN_RESERVED_00000002 = 0x00000002,  // 保留，必须为 0
    IMAGE_SCN_RESERVED_00000004 = 0x00000004,  // 保留，必须为 0

    // 节内容类型
    IMAGE_SCN_TYPE_NO_PAD       = 0x00000008,  // 不进行字节填充对齐
    IMAGE_SCN_CNT_CODE          = 0x00000020,  // 包含代码
    IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040, // 包含已初始化数据
    IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080, // 包含未初始化数据

    // 节的特性
    IMAGE_SCN_LNK_OTHER         = 0x00000100,  // 保留，必须为 0
    IMAGE_SCN_LNK_INFO          = 0x00000200,  // 包含注释或其他信息
    IMAGE_SCN_LNK_REMOVE        = 0x00000800,  // 在链接后可移除
    IMAGE_SCN_LNK_COMDAT        = 0x00001000,  // 包含 COMDAT 数据

    // 内存布局
    IMAGE_SCN_GPREL             = 0x00008000,  // 包含全局指针相对数据
    IMAGE_SCN_MEM_FARDATA       = 0x00008000,  // 远程数据段
    IMAGE_SCN_MEM_PURGEABLE     = 0x00020000,  // 可丢弃
    IMAGE_SCN_MEM_16BIT         = 0x00020000,  // 16 位段
    IMAGE_SCN_MEM_LOCKED        = 0x00040000,  // 已锁定段
    IMAGE_SCN_MEM_PRELOAD       = 0x00080000,  // 预加载段

    // 节权限
    IMAGE_SCN_ALIGN_1BYTES      = 0x00100000,  // 1 字节对齐
    IMAGE_SCN_ALIGN_2BYTES      = 0x00200000,  // 2 字节对齐
    IMAGE_SCN_ALIGN_4BYTES      = 0x00300000,  // 4 字节对齐
    IMAGE_SCN_ALIGN_8BYTES      = 0x00400000,  // 8 字节对齐
    IMAGE_SCN_ALIGN_16BYTES     = 0x00500000,  // 16 字节对齐
    IMAGE_SCN_ALIGN_32BYTES     = 0x00600000,  // 32 字节对齐
    IMAGE_SCN_ALIGN_64BYTES     = 0x00700000,  // 64 字节对齐
    IMAGE_SCN_ALIGN_128BYTES    = 0x00800000,  // 128 字节对齐
    IMAGE_SCN_ALIGN_256BYTES    = 0x00900000,  // 256 字节对齐
    IMAGE_SCN_ALIGN_512BYTES    = 0x00A00000,  // 512 字节对齐
    IMAGE_SCN_ALIGN_1024BYTES   = 0x00B00000,  // 1024 字节对齐
    IMAGE_SCN_ALIGN_2048BYTES   = 0x00C00000,  // 2048 字节对齐
    IMAGE_SCN_ALIGN_4096BYTES   = 0x00D00000,  // 4096 字节对齐
    IMAGE_SCN_ALIGN_8192BYTES   = 0x00E00000,  // 8192 字节对齐

    IMAGE_SCN_LNK_NRELOC_OVFL   = 0x01000000,  // 重定位条目数超过 16 位限制
    IMAGE_SCN_MEM_DISCARDABLE   = 0x02000000,  // 可丢弃节
    IMAGE_SCN_MEM_NOT_CACHED    = 0x04000000,  // 不可缓存
    IMAGE_SCN_MEM_NOT_PAGED     = 0x08000000,  // 不可分页
    IMAGE_SCN_MEM_SHARED        = 0x10000000,  // 可共享
    IMAGE_SCN_MEM_EXECUTE       = 0x20000000,  // 可执行
    IMAGE_SCN_MEM_READ          = 0x40000000,  // 可读
    IMAGE_SCN_MEM_WRITE         = 0x80000000   // 可写
} SectionCharacteristics;


// 定义节表结构
typedef struct {
    char name[8];                // 节名称
    uint32_t virtualSize;        // 节的虚拟大小
    uint32_t virtualAddress;     // 节的虚拟地址
    uint32_t sizeOfRawData;      // 节在文件中的大小
    uint32_t pointerToRawData;   // 节在文件中的偏移
    uint32_t pointerToRelocations; // 重定位表的偏移（通常为 0）
    uint32_t pointerToLineNumbers; // 行号表偏移（通常为 0）
    uint16_t numberOfRelocations; // 重定位表条目数
    uint16_t numberOfLineNumbers; // 行号表条目数
    uint32_t characteristics;    // 节的标志（如可读、可执行）
} SectionHeader;


// 定义段信息结构（自定义，用于模拟段的描述）
typedef struct {
    uint32_t type;               // 段类型（模拟 ELF 的段类型）
    uint32_t flags;              // 段标志（可读、可写、可执行）
    uint32_t fileOffset;         // 段在文件中的偏移
    uint32_t virtualAddress;     // 段的虚拟地址
    uint32_t fileSize;           // 段在文件中的大小
    uint32_t memorySize;         // 段在内存中的大小
    uint32_t alignment;          // 对齐方式
} ProgramSegment;

size_t getPeHeaderSize();

// 创建并填充 PE 头部
void *createPeHeader(uint16_t machine,
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
