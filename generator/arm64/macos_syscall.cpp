//
// Created by Park Yu on 2024/11/21.
//

#include "macos_syscall.h"
#include "binary_arm64.h"
#include "register_arm64.h"

void initMachOProgramStart() {
    emitLabel("_start");
    binaryOp2(INST_MOV, 1, X29, 0, true);
    binaryOp2(INST_MOV, 1, X30, 0, true);
    binaryOpBranch(INST_BL, UNUSED, "main");
    //keep the "main" ret value in X0
    binaryOp2(INST_MOV, 1, X16, MACOS_SYS_EXIT, true);
    binaryOpSvc(INST_SVC, 0x80);
}