//
// Created by Park Yu on 2024/10/10.
//

#include "elf.h"
#include "file.h"
#include "mspace.h"
#include <string.h>

static const char *ELF_TAG = "elf";

void writeElfHeader() {
    // 初始化 ELF 文件头
    Elf64_Ehdr *header = (Elf64_Ehdr *) pccMalloc(ELF_TAG, sizeof(Elf64_Ehdr));
    // 设置 e_ident 字段
    memset(header->e_ident, 0, sizeof(header->e_ident));
    header->e_ident[0] = 0x7f;      // 魔术字节
    header->e_ident[1] = 'E';
    header->e_ident[2] = 'L';
    header->e_ident[3] = 'F';
    header->e_ident[4] = 2;         // 64 位
    header->e_ident[5] = 1;         // 小端
    header->e_ident[6] = 1;         // ELF 版本

    // 设置其他字段
    header->e_type = 2;             // 可执行文件
    header->e_machine = 0xB7;       // ARM64 架构
    header->e_version = 1;          // ELF 版本
    header->e_entry = 0x400000;     // 程序入口点地址（通常由链接器设置）
    header->e_phoff = 64;           // 程序头表偏移（ELF 头大小通常是 64 字节）
    header->e_shoff = 0;            // 暂时设为 0，稍后由链接器设置
    header->e_flags = 0;            // ARM64 通常无特定标志
    header->e_ehsize = sizeof(Elf64_Ehdr);  // ELF 头大小
    header->e_phentsize = 56;       // 程序头表项的大小，64 位系统通常为 56 字节
    header->e_phnum = 1;            // 程序头表项数量（通常至少有一个）
    header->e_shentsize = 64;       // 节头表项的大小（64 字节）
    header->e_shnum = 0;            // 暂时设为 0，稍后由链接器设置
    header->e_shstrndx = 0;         // 节名称字符串表的索引

    writeFileB(header, sizeof(Elf64_Ehdr));
}

void writeElf64() {
    openFile("test_elf");
    writeElfHeader();

    closeFile();
}