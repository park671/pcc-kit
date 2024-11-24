//
// Created by Park Yu on 2024/10/10.
//

#include "elf.h"
#include "file.h"
#include "mspace.h"
#include "assembler.h"
#include "logger.h"

static const char *ELF_TAG = "elf";

// 创建并填充 ELF 头部
Elf64_Ehdr *createElfHeader(uint16_t type,
                            uint16_t machine,
                            uint64_t entry,
                            uint64_t phoff,
                            uint64_t shoff,
                            uint16_t phentsize,
                            uint16_t phnum,
                            uint16_t shentsize,
                            uint16_t shnum,
                            uint16_t shstrndx) {
    // 分配内存
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *) malloc(sizeof(Elf64_Ehdr));

    // 检查分配是否成功
    if (ehdr == nullptr) {
        printf("Memory allocation failed\n");
        return nullptr;
    }

    // 填充 e_ident 魔术字节和标识信息
    ehdr->e_ident[0] = 0x7f;    // ELF 魔术字节
    ehdr->e_ident[1] = 'E';
    ehdr->e_ident[2] = 'L';
    ehdr->e_ident[3] = 'F';
    ehdr->e_ident[4] = 2;       // 64位
    ehdr->e_ident[5] = 1;       // 小端序
    ehdr->e_ident[6] = 1;       // ELF 版本
    ehdr->e_ident[7] = 3;       // linux abi
    for (int i = 8; i < 16; i++) {
        ehdr->e_ident[i] = 0;   // 填充其余部分
    }

    // 填充其他字段
    ehdr->e_type = type;            // 文件类型
    ehdr->e_machine = machine;      // 目标机器类型
    ehdr->e_version = 1;            // ELF 版本
    ehdr->e_entry = entry;          // 程序入口点地址
    ehdr->e_phoff = phoff;          // 程序头表偏移
    ehdr->e_shoff = shoff;          // 节头表偏移
    ehdr->e_flags = 0;              // 无特殊标志
    ehdr->e_ehsize = sizeof(Elf64_Ehdr);  // ELF 头大小
    ehdr->e_phentsize = phentsize;  // 程序头表项大小
    ehdr->e_phnum = phnum;          // 程序头表项数量
    ehdr->e_shentsize = shentsize;  // 节头表项大小
    ehdr->e_shnum = shnum;          // 节头表项数量
    ehdr->e_shstrndx = shstrndx;    // 节名称字符串表索引

    return ehdr;  // 返回已填充的 ELF 头部指针
}

Elf64_Phdr *createProgramHeader(uint32_t type,
                                uint32_t flags,
                                uint64_t offset,
                                uint64_t vaddr,
                                uint64_t paddr,
                                uint64_t filesz,
                                uint64_t memsz,
                                uint64_t align) {
    // 分配内存
    Elf64_Phdr *phdr = (Elf64_Phdr *) malloc(sizeof(Elf64_Phdr));

    // 检查分配是否成功
    if (phdr == nullptr) {
        loge(ELF_TAG, "memory allocation failed\n");
        return nullptr;
    }

    // 填充结构体字段
    phdr->p_type = type;          // 设置段类型
    phdr->p_flags = flags;        // 设置段标志（可读、可写、可执行等）
    phdr->p_offset = offset;      // 设置段在文件中的偏移
    phdr->p_vaddr = vaddr;        // 设置段的虚拟地址
    phdr->p_paddr = paddr;        // 设置段的物理地址
    phdr->p_filesz = filesz;      // 设置段在文件中的大小
    phdr->p_memsz = memsz;        // 设置段在内存中的大小
    phdr->p_align = align;        // 设置段的对齐要求

    return phdr;  // 返回已填充的程序头指针
}

// 创建并填充节头的方法
Elf64_Shdr *createSectionHeader(uint32_t name,
                                uint32_t type,
                                uint64_t flags,
                                uint64_t addr,
                                uint64_t offset,
                                uint64_t size,
                                uint32_t link,
                                uint32_t info,
                                uint64_t addralign,
                                uint64_t entsize) {
    // 分配节头的内存
    Elf64_Shdr *shdr = (Elf64_Shdr *) malloc(sizeof(Elf64_Shdr));

    // 检查分配是否成功
    if (shdr == nullptr) {
        loge(ELF_TAG, "Error: memory allocation failed\n");
        return nullptr;
    }

    // 填充节头字段
    shdr->sh_name = name;              // 设置节名称（字符串表索引）
    shdr->sh_type = type;              // 设置节类型
    shdr->sh_flags = flags;            // 设置节标志（如可读、可写、可执行）
    shdr->sh_addr = addr;              // 设置节在内存中的虚拟地址
    shdr->sh_offset = offset;          // 设置节在文件中的偏移
    shdr->sh_size = size;              // 设置节的大小
    shdr->sh_link = link;              // 设置相关节的索引
    shdr->sh_info = info;              // 设置额外信息
    shdr->sh_addralign = addralign;    // 设置节的对齐要求
    shdr->sh_entsize = entsize;        // 设置表项的大小

    return shdr;
}