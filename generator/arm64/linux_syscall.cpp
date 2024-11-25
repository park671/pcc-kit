//
// Created by Park Yu on 2024/11/21.
//
#include "linux_syscall.h"

void initLinuxArm64ProgramStart() {
    emitLabel("_start");
    binaryOp2(INST_MOV, 1, X29, 0, true);
    binaryOp2(INST_MOV, 1, X30, 0, true);
    binaryOpBranch(INST_BL, UNUSED, "main");
    //keep the "main" ret value in X0
    binaryOp2(INST_MOV, 1, X8, SYS_EXIT, true);
    binaryOpSvc(INST_SVC, 0);
}

//write(int fd, void *buffer, int size);
void initLinuxArm64SyscallWrapper() {
    emitLabel("write");
    binaryOp2(INST_MOV, 1, X8, SYS_WRITE, true);
    binaryOpSvc(INST_SVC, 0);
    binaryOpRet(INST_RET);
    emitLabel("read");
    binaryOp2(INST_MOV, 1, X8, SYS_READ, true);
    binaryOpSvc(INST_SVC, 0);
    binaryOpRet(INST_RET);
}
