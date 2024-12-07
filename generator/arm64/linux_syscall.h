//
// Created by Park Yu on 2024/10/17.
//

#ifndef PCC_LINUX_SYSCALL_H
#define PCC_LINUX_SYSCALL_H

#include "binary_arm64.h"
#include "register_arm64.h"

enum LinuxArm64SysCallNumber{
    LINUX_ARM64_SYS_READ           = 63,
    LINUX_ARM64_SYS_WRITE          = 64,
    LINUX_ARM64_SYS_OPEN           = 56,
    LINUX_ARM64_SYS_CLOSE          = 57,
    LINUX_ARM64_SYS_STAT           = 79,
    LINUX_ARM64_SYS_FSTAT          = 80,
    LINUX_ARM64_SYS_LSTAT          = 81,
    LINUX_ARM64_SYS_POLL           = 73,
    LINUX_ARM64_SYS_LSEEK          = 62,
    LINUX_ARM64_SYS_MMAP           = 222,
    LINUX_ARM64_SYS_MPROTECT       = 226,
    LINUX_ARM64_SYS_MUNMAP         = 215,
    LINUX_ARM64_SYS_BRK            = 214,
    LINUX_ARM64_SYS_RT_SIGACTION   = 134,
    LINUX_ARM64_SYS_RT_SIGPROCMASK = 135,
    LINUX_ARM64_SYS_IOCTL          = 29,
    LINUX_ARM64_SYS_PREAD64        = 67,
    LINUX_ARM64_SYS_PWRITE64       = 68,
    LINUX_ARM64_SYS_READV          = 65,
    LINUX_ARM64_SYS_WRITEV         = 66,
    LINUX_ARM64_SYS_EXIT           = 93,
    LINUX_ARM64_SYS_EXIT_GROUP     = 94,
    LINUX_ARM64_SYS_CLONE          = 220,
    LINUX_ARM64_SYS_FORK           = 220,  // 在 AArch64 上，fork 使用 clone 实现
    LINUX_ARM64_SYS_VFORK          = 220,  // vfork 同样使用 clone 实现
    LINUX_ARM64_SYS_EXECVE         = 221,
    LINUX_ARM64_SYS_WAIT4          = 260,
    LINUX_ARM64_SYS_KILL           = 129,
    // 根据需要添加其他系统调用
};

void initLinuxArm64ProgramStart();
void initLinuxArm64SyscallWrapper();

#endif //PCC_LINUX_SYSCALL_H
