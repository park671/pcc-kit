//
// Created by Park Yu on 2024/10/10.
//

#include "macho.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *MACHO_TAG = "macho";

mach_header_64 *createMachHeader64(uint32_t cputype,
                                   uint32_t cpusubtype,
                                   uint32_t filetype,
                                   uint32_t ncmds,
                                   uint32_t sizeofcmds,
                                   uint32_t flags) {
    // 分配内存
    mach_header_64 *header = (mach_header_64 *) malloc(sizeof(mach_header_64));

    // 检查分配是否成功
    if (header == nullptr) {
        printf("Memory allocation failed\n");
        return nullptr;
    }

    // 填充 Mach-O 头部字段
    header->magic = MH_MAGIC_64;       // 64 位 Mach-O 魔数
    header->cputype = cputype;         // CPU 类型
    header->cpusubtype = cpusubtype;   // CPU 子类型
    header->filetype = filetype;       // 文件类型（可执行文件、动态库等）
    header->ncmds = ncmds;             // 加载命令数量
    header->sizeofcmds = sizeofcmds;   // 加载命令的总大小
    header->flags = flags;             // 标志
    header->reserved = 0;              // 保留字段（通常为 0）

    return header; // 返回已填充的 Mach-O 头部指针
}

segment_command_64 *createSegmentCommand64(const char *segname,
                                           uint64_t vmaddr,
                                           uint64_t vmsize,
                                           uint64_t fileoff,
                                           uint64_t filesize,
                                           uint32_t maxprot,
                                           uint32_t initprot,
                                           uint32_t nsects,
                                           uint32_t flags) {
    // 分配内存
    segment_command_64 *segment = (segment_command_64 *) malloc(sizeof(segment_command_64));

    // 检查分配是否成功
    if (segment == nullptr) {
        printf("Memory allocation failed\n");
        return nullptr;
    }

    // 填充段命令字段
    segment->cmd = LC_SEGMENT_64;       // 加载命令类型
    segment->cmdsize = sizeof(segment_command_64); // 命令大小
    strncpy(segment->segname, segname, 16); // 段名称（最多 16 字节）
    segment->vmaddr = vmaddr;          // 虚拟地址
    segment->vmsize = vmsize;          // 虚拟内存大小
    segment->fileoff = fileoff;        // 文件偏移
    segment->filesize = filesize;      // 文件大小
    segment->maxprot = maxprot;        // 最大权限
    segment->initprot = initprot;      // 初始化权限
    segment->nsects = nsects;          // 区块数量
    segment->flags = flags;            // 段标志

    return segment; // 返回已填充的段命令指针
}

symtab_command *createSymtabCommand(uint32_t symoff,
                                    uint32_t nsyms,
                                    uint32_t stroff,
                                    uint32_t strsize) {
    // 分配内存
    symtab_command *symtab = (symtab_command *) malloc(sizeof(symtab_command));

    // 检查分配是否成功
    if (symtab == nullptr) {
        printf("Memory allocation failed\n");
        return nullptr;
    }

    // 填充符号表命令字段
    symtab->cmd = LC_SYMTAB;           // 加载命令类型
    symtab->cmdsize = sizeof(symtab_command); // 命令大小
    symtab->symoff = symoff;           // 符号表偏移
    symtab->nsyms = nsyms;             // 符号数量
    symtab->stroff = stroff;           // 字符串表偏移
    symtab->strsize = strsize;         // 字符串表大小

    return symtab; // 返回已填充的符号表命令指针
}

section_64 *createSection64(const char *sectname,
                            const char *segname,
                            uint64_t addr,
                            uint64_t size,
                            uint32_t offset,
                            uint32_t align,
                            uint32_t reloff,
                            uint32_t nreloc,
                            uint32_t flags) {
    // 分配内存
    section_64 *section = (section_64 *) malloc(sizeof(section_64));

    // 检查分配是否成功
    if (section == nullptr) {
        printf("Memory allocation failed\n");
        return nullptr;
    }

    // 填充区块字段
    strncpy(section->sectname, sectname, 16); // 区块名称
    strncpy(section->segname, segname, 16);   // 所属段名称
    section->addr = addr;                     // 区块虚拟地址
    section->size = size;                     // 区块大小
    section->offset = offset;                 // 文件偏移
    section->align = align;                   // 对齐要求
    section->reloff = reloff;                 // 重定位表偏移
    section->nreloc = nreloc;                 // 重定位项数量
    section->flags = flags;                   // 区块标志
    section->reserved1 = 0;                   // 保留字段 1
    section->reserved2 = 0;                   // 保留字段 2
    section->reserved3 = 0;                   // 保留字段 3

    return section; // 返回已填充的区块指针
}

entry_point_command *createEntryPointCommand(uint64_t entryoff, uint64_t stacksize) {
    // 分配内存
    entry_point_command *entryPointCmd = (entry_point_command *) malloc(sizeof(entry_point_command));

    // 检查分配是否成功
    if (entryPointCmd == nullptr) {
        loge(MACHO_TAG, "Error: Memory allocation failed\n");
        return nullptr;
    }

    // 填充 entry_point_command 的字段
    entryPointCmd->cmd = LC_MAIN;           // 设置命令类型为 LC_MAIN
    entryPointCmd->cmdsize = sizeof(entry_point_command); // 命令大小为 24 字节
    entryPointCmd->entryoff = entryoff;     // 程序入口偏移（从 __TEXT 段的起始位置计算）
    entryPointCmd->stacksize = stacksize;   // 初始栈大小（如果为 0，则使用默认值）

    return entryPointCmd; // 返回已填充的 entry_point_command 指针
}
