//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_ELF_H
#define PCC_ELF_H

#include <stdint.h>

enum ElfType {
    ET_NONE,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    ET_CORE,
};

enum ElfArch{
    EM_NONE      = 0,    // No machine
    EM_M32       = 1,    // AT&T WE 32100
    EM_SPARC     = 2,    // SPARC
    EM_386       = 3,    // Intel 80386
    EM_68K       = 4,    // Motorola 68000
    EM_PPC       = 20,   // PowerPC
    EM_ARM       = 40,   // ARM 32-bit
    EM_IA_64     = 50,   // Intel IA-64 (Itanium)
    EM_X86_64    = 62,   // AMD x86-64 architecture (64-bit)
    EM_AARCH64   = 183,  // ARM 64-bit architecture (AArch64)
    EM_RISCV     = 243,  // RISC-V architecture
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

extern void writeElf64();

#endif //PCC_ELF_H
