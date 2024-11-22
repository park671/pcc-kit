//
// Created by Park Yu on 2024/11/21.
//
#include "windows_syscall.h"

void initWindowsArm64ProgramStart() {
    emitLabel("_start");
    binaryOp2(INST_MOV, 1, X29, 0, true);
    binaryOp2(INST_MOV, 1, X30, 0, true);
    binaryOpBranch(INST_BL, UNUSED, "main");
    //move the "main" ret value to X1, set X0 with "invalid handle"
    //NtTerminateProcess(null, returnValue);
    binaryOp2(INST_MOV, 1, X1, X0, false);
    binaryOp2(INST_MOV, 1, X0, INVALID_HANDLE_VALUE, true);
    binaryOpSvc(INST_SVC, NtTerminateProcess);
}