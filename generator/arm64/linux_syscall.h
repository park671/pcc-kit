//
// Created by Park Yu on 2024/10/17.
//

#ifndef PCC_LINUX_SYSCALL_H
#define PCC_LINUX_SYSCALL_H

#include "binary_arm64.h"
#include "register_arm64.h"

enum LinuxArm64SysCallNumber{
    SYS_READ           = 63,
    SYS_WRITE          = 64,
    SYS_OPEN           = 56,
    SYS_CLOSE          = 57,
    SYS_STAT           = 79,
    SYS_FSTAT          = 80,
    SYS_LSTAT          = 81,
    SYS_POLL           = 73,
    SYS_LSEEK          = 62,
    SYS_MMAP           = 222,
    SYS_MPROTECT       = 226,
    SYS_MUNMAP         = 215,
    SYS_BRK            = 214,
    SYS_RT_SIGACTION   = 134,
    SYS_RT_SIGPROCMASK = 135,
    SYS_IOCTL          = 29,
    SYS_PREAD64        = 67,
    SYS_PWRITE64       = 68,
    SYS_READV          = 65,
    SYS_WRITEV         = 66,
    SYS_EXIT           = 93,
    SYS_EXIT_GROUP     = 94,
    SYS_CLONE          = 220,
    SYS_FORK           = 220,  // 在 AArch64 上，fork 使用 clone 实现
    SYS_VFORK          = 220,  // vfork 同样使用 clone 实现
    SYS_EXECVE         = 221,
    SYS_WAIT4          = 260,
    SYS_KILL           = 129,
    // 根据需要添加其他系统调用
};

void initLinuxArm64ProgramStart();

#endif //PCC_LINUX_SYSCALL_H
