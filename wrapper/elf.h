//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_ELF_H
#define PCC_ELF_H

#include <stdint.h>

typedef struct {
    unsigned char e_ident[16];    // ELF 魔术字节和其他标识信息
    uint16_t e_type;              // 文件类型（可执行文件、共享库等）
    uint16_t e_machine;           // 目标机器类型，ARM64 为 0xB7
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

extern void writeElf64();

#endif //PCC_ELF_H
