//
// Created by Park Yu on 2024/11/21.
//
#include "linux_syscall.h"
#include "logger.h"

const char *LINUX_SYSCALL_TAG = "linux_syscall";

void initLinuxArm64ProgramStart() {
    logd(LINUX_SYSCALL_TAG, "static link program start...");
    emitLabel("_start");
    binaryOp2(INST_MOV, 1, X29, 0, true);
    binaryOp2(INST_MOV, 1, X30, 0, true);
    binaryOpBranch(INST_BL, UNUSED, "main");
    //keep the "main" ret value in X0
    binaryOp2(INST_MOV, 1, X8, LINUX_ARM64_SYS_EXIT, true);
    binaryOpSvc(INST_SVC, 0);
}

//write(int fd, void *buffer, int size);
void initLinuxArm64SyscallWrapper() {
    logd(LINUX_SYSCALL_TAG, "static link syscall method...");
    emitLabel("write");
    binaryOp2(INST_MOV, 1, X8, LINUX_ARM64_SYS_WRITE, true);
    binaryOpSvc(INST_SVC, 0);
    binaryOpRet(INST_RET);
    emitLabel("read");
    binaryOp2(INST_MOV, 1, X8, LINUX_ARM64_SYS_READ, true);
    binaryOpSvc(INST_SVC, 0);
    binaryOpRet(INST_RET);
    emitLabel("fork");
    binaryOp2(INST_MOV, 1, X8, LINUX_ARM64_SYS_FORK, true);
    binaryOpSvc(INST_SVC, 0);
    binaryOpRet(INST_RET);
}
