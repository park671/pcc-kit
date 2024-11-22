//
// Created by Park Yu on 2024/11/21.
//

#ifndef PCC_MACOS_SYSCALL_H
#define PCC_MACOS_SYSCALL_H

enum MacOsArm64SysCallNumber {
    MACOS_SYS_SYSCALL = 0,
    MACOS_SYS_EXIT = 1,
    MACOS_SYS_FORK = 2,
    MACOS_SYS_READ = 3,
    MACOS_SYS_WRITE = 4,
    MACOS_SYS_OPEN = 5,
    MACOS_SYS_CLOSE = 6
};

void initMachOProgramStart();

#endif //PCC_MACOS_SYSCALL_H
