//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_ELF_H
#define PCC_ELF_H

#include <stdint.h>
#include "assembler.h"

enum ElfType {
    ET_NONE,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    ET_CORE,
};

enum ElfArch {
    EM_NONE = 0,    // No machine
    EM_M32 = 1,    // AT&T WE 32100
    EM_SPARC = 2,    // SPARC
    EM_386 = 3,    // Intel 80386
    EM_68K = 4,    // Motorola 68000
    EM_PPC = 20,   // PowerPC
    EM_ARM = 40,   // ARM 32-bit
    EM_IA_64 = 50,   // Intel IA-64 (Itanium)
    EM_X86_64 = 62,   // AMD x86-64 architecture (64-bit)
    EM_AARCH64 = 183,  // ARM 64-bit architecture (AArch64)
    EM_RISCV = 243,  // RISC-V architecture
};


// 定义 ELF 文件头的结构体
typedef struct {
    unsigned char e_ident[16];    // ELF 魔术字节和其他标识信息
    uint16_t e_type;              // 文件类型
    uint16_t e_machine;           // 目标机器类型
    uint32_t e_version;           // 版本信息
    uint64_t e_entry;             // 程序入口点地址
    uint64_t e_phoff;             // 程序头表偏移
    uint64_t e_shoff;             // 节头表偏移
    uint32_t e_flags;             // 特定于体系结构的标志
    uint16_t e_ehsize;            // ELF 头大小
    uint16_t e_phentsize;         // 每个程序头表项的大小
    uint16_t e_phnum;             // 程序头表中的表项数量
    uint16_t e_shentsize;         // 每个节头表项的大小
    uint16_t e_shnum;             // 节头表中的表项数量
    uint16_t e_shstrndx;          // 节名称字符串表的索引
} Elf64_Ehdr;

// 定义程序头的结构体
typedef struct {
    uint32_t p_type;              // 段类型
    uint32_t p_flags;             // 段标志
    uint64_t p_offset;            // 段在文件中的偏移
    uint64_t p_vaddr;             // 段的虚拟地址
    uint64_t p_paddr;             // 段的物理地址
    uint64_t p_filesz;            // 段在文件中的大小
    uint64_t p_memsz;             // 段在内存中的大小
    uint64_t p_align;             // 段的对齐
} Elf64_Phdr;

// 定义节头的结构体
typedef struct {
    uint32_t sh_name;             // 节名称（索引）
    uint32_t sh_type;             // 节类型
    uint64_t sh_flags;            // 节标志
    uint64_t sh_addr;             // 节在内存中的虚拟地址
    uint64_t sh_offset;           // 节在文件中的偏移
    uint64_t sh_size;             // 节的大小
    uint32_t sh_link;             // 相关节的索引
    uint32_t sh_info;             // 额外信息
    uint64_t sh_addralign;        // 地址对齐
    uint64_t sh_entsize;          // 表项大小
} Elf64_Shdr;

// ELF 符号表条目结构体
typedef struct {
    uint32_t st_name;             // 符号名称索引
    unsigned char st_info;        // 符号类型和绑定信息
    unsigned char st_other;       // 符号其他信息
    uint16_t st_shndx;            // 符号所在节的索引
    uint64_t st_value;            // 符号的值（地址）
    uint64_t st_size;             // 符号的大小
} Elf64_Sym;

extern Elf64_Ehdr *createElfHeader(uint16_t type,
                                   uint16_t machine,
                                   uint64_t entry,
                                   uint64_t phoff,
                                   uint64_t shoff,
                                   uint16_t phentsize,
                                   uint16_t phnum,
                                   uint16_t shentsize,
                                   uint16_t shnum,
                                   uint16_t shstrndx);

// Program header type enums
enum ElfProgramHeaderType {
    PT_NULL = 0,         // 未使用的条目
    PT_LOAD = 1,         // 可加载段
    PT_DYNAMIC = 2,         // 动态链接信息
    PT_INTERP = 3,         // 程序解释器路径
    PT_NOTE = 4,         // 附加信息
    PT_SHLIB = 5,         // 保留，未使用
    PT_PHDR = 6,         // 程序头表自身的位置
    PT_TLS = 7,         // 线程局部存储段
    PT_LOOS = 0x60000000,// 操作系统专用段的开始
    PT_HIOS = 0x6FFFFFFF,// 操作系统专用段的结束
    PT_LOPROC = 0x70000000,// 处理器专用段的开始
    PT_HIPROC = 0x7FFFFFFF // 处理器专用段的结束
};

enum ElfProgramFlags {
    PF_X = 0x1,  // 段是可执行的
    PF_W = 0x2,  // 段是可写的
    PF_R = 0x4,  // 段是可读的
};

extern Elf64_Phdr *createProgramHeader(uint32_t type,
                                       uint32_t flags,
                                       uint64_t offset,
                                       uint64_t vaddr,
                                       uint64_t paddr,
                                       uint64_t filesz,
                                       uint64_t memsz,
                                       uint64_t align);

// 定义 sh_type 的常见枚举
enum ElfSectionType {
    SHT_NULL = 0,        // 节头表项无效
    SHT_PROGBITS = 1,        // 程序数据
    SHT_SYMTAB = 2,        // 符号表
    SHT_STRTAB = 3,        // 字符串表
    SHT_RELA = 4,        // 重定位表 (带显式添加)
    SHT_HASH = 5,        // 符号表的哈希表
    SHT_DYNAMIC = 6,        // 动态链接信息
    SHT_NOTE = 7,        // 附加信息
    SHT_NOBITS = 8,        // 没有文件内容的节（如 .bss）
    SHT_REL = 9,        // 重定位表 (不带显式添加)
    SHT_DYNSYM = 11        // 动态符号表
};

// 定义 sh_flags 的常见枚举
enum ElfSectionFlags {
    SHF_WRITE = 0x1,     // 节可写
    SHF_ALLOC = 0x2,     // 节占用内存
    SHF_EXECINSTR = 0x4,     // 节包含可执行指令
    SHF_MERGE = 0x10,    // 节中可合并条目
    SHF_STRINGS = 0x20,    // 节包含字符串
    SHF_INFO_LINK = 0x40,    // sh_info 字段含有额外信息
    SHF_LINK_ORDER = 0x80,   // 节按照sh_link给出的顺序排列
};

extern Elf64_Shdr *createSectionHeader(uint32_t name,
                                       uint32_t type,
                                       uint64_t flags,
                                       uint64_t addr,
                                       uint64_t offset,
                                       uint64_t size,
                                       uint32_t link,
                                       uint32_t info,
                                       uint64_t addralign,
                                       uint64_t entsize);

static const char *shstrtabString = "\0"
                                    ".interp\0"
                                    ".text\0"
                                    ".data\0"
                                    ".rodata\0"
                                    ".bss\0"
                                    ".shstrtab\0"
                                    ".strtab\0";

static const int shstrtabSize = 52;

//0.interp0.text0.data0.rodata0.bss0.shstrtab0.strtab0

static const int INTERP_SECTION_IDX = 1;
static const int TEXT_SECTION_IDX = 9;
static const int DATA_SECTION_IDX = 15;
static const int RODATA_SECTION_IDX = 21;
static const int BSS_SECTION_IDX = 29;
static const int SHSTRTAB_SECTION_IDX = 34;
static const int STRTAB_SECTION_IDX = 44;

#endif //PCC_ELF_H
