//
// Created by Park Yu on 2024/9/23.
//

#ifndef PCC_CC_BIN_ARM64_H
#define PCC_CC_BIN_ARM64_H

#include <stdint.h>

typedef uint32_t Operand;

typedef uint32_t Inst;

enum Arm64Inst {
    unknown,

    mul,
    add,
    sub,
    sdiv,
    mod,//fake inst

    stp,
    ldp,
    str,
    ldr,

    cmp,
    bc,//b.cc

    b,//b
    bl,

    mov,

    ret
};

enum BranchCondition {
    EQ = 0x0,  // Equal (Z == 1)
    NE = 0x1,  // Not equal (Z == 0)
    LT = 0xb,  // Signed less than (N != V)
    LE = 0xd,  // Signed less than or equal (Z == 1 || N != V)
    GT = 0xc,  // Signed greater than (Z == 0 && N == V)
    GE = 0xa,  // Signed greater than or equal (N == V)
    CS = 0x2,  // Unsigned higher or same (C == 1)
    CC = 0x3,  // Unsigned lower (C == 0)
    VS = 0x6,  // Overflow (V == 1)
    VC = 0x7,  // No overflow (V == 0)

    UNUSED = 0xFF,
};

struct InstBuffer {
    Inst *result;
    int size;
};

void emitLabel(const char *label);
void relocate(int32_t baseAddr);
InstBuffer *getEmittedInstBuffer();

/**
 * add sub mul div mod
 */
void binaryOp3(Arm64Inst inst, bool halfWidth, Operand x, Operand a, Operand b, bool bImm);

/**
 * stp ldp, str ldr
 */
void binaryOpStoreLoad(Arm64Inst inst, bool halfWidth, Operand reg1, Operand reg2, Operand baseReg, int32_t offset);

/**
 * mov cmp
 */
void binaryOp2(Arm64Inst inst, bool halfWidth, Operand dest, Operand src, bool srcImm);

/**
 * b.cc, b, bl
 */
void binaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, const char *label);

/**
 * ret
 */
void binaryOpRet(Arm64Inst inst);

#endif //PCC_CC_BIN_ARM64_H
