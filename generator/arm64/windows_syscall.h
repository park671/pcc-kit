//
// Created by Park Yu on 2024/10/17.
//

#ifndef PCC_LINUX_WINDOWS_SYSCALL_H
#define PCC_LINUX_WINDOWS_SYSCALL_H

#include "binary_arm64.h"
#include "register_arm64.h"

enum WindowsArm64SysCallNumber {
    NtWriteFile = 8,
    NtTerminateProcess = 44,
    NtTerminateThread = 83,
    // 根据需要添加其他系统调用
};

//00 00 80 92

#define INVALID_HANDLE_VALUE -1

void initWindowsArm64ProgramStart();

#endif //PCC_LINUX_WINDOWS_SYSCALL_H
