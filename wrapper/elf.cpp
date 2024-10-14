//
// Created by Park Yu on 2024/10/10.
//

#include "elf.h"
#include "file.h"
#include "mspace.h"
#include <string.h>
#include "assembler.h"
#include "logger.h"

static const char *ELF_TAG = "elf";

Elf64_Ehdr *writeElfHeader(
        int shared,
        Arch arch
) {
    Elf64_Ehdr *header = (Elf64_Ehdr *) pccMalloc(ELF_TAG, sizeof(Elf64_Ehdr));
    memset(header->e_ident, 0, sizeof(header->e_ident));
    header->e_ident[0] = 0x7f;
    header->e_ident[1] = 'E';
    header->e_ident[2] = 'L';
    header->e_ident[3] = 'F';
    header->e_ident[4] = 2;
    header->e_ident[5] = 1;
    header->e_ident[6] = 1;

    if (shared) {
        header->e_type = ET_DYN;
    } else {
        header->e_type = ET_EXEC;
    }
    switch (arch) {
        case ARCH_ARM64: {
            header->e_machine = EM_AARCH64;
            break;
        }
        case ARCH_X86_64: {
            header->e_machine = EM_X86_64;
            break;
        }
        default: {
            loge(ELF_TAG, "unknown arch:%d", arch);
            return nullptr;
        }
    }
    header->e_version = 1;          // ELF 版本
    header->e_entry = 0;     // 程序入口点地址（通常由链接器设置）
    header->e_phoff = 0;           // 程序头表偏移（ELF 头大小通常是 64 字节）
    header->e_shoff = 0;            // 暂时设为 0，稍后由链接器设置
    header->e_flags = 0;            // ARM64 通常无特定标志
    header->e_ehsize = sizeof(Elf64_Ehdr);  // ELF 头大小
    header->e_phentsize = 56;       // 程序头表项的大小，64 位系统通常为 56 字节
    header->e_phnum = 1;            // 程序头表项数量（通常至少有一个）
    header->e_shentsize = 64;       // 节头表项的大小（64 字节）
    header->e_shnum = 0;            // 暂时设为 0，稍后由链接器设置
    header->e_shstrndx = 0;         // 节名称字符串表的索引
    return header;
}

void writeElf64() {
    openFile("test_elf");
    writeElfHeader();

    closeFile();
}