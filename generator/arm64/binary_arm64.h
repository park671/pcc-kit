//
// Created by Park Yu on 2024/9/23.
//

#ifndef PCC_CC_BIN_ARM64_H
#define PCC_CC_BIN_ARM64_H

#include <stdint.h>

typedef int64_t Operand;

typedef uint32_t Inst;

enum Arm64Inst {
    INST_UNKNOWN,

    INST_MUL,
    INST_ADD,
    INST_SUB,
    INST_SDIV,
    INST_MOD,

    INST_AND,
    INST_OR,

    INST_STP,
    INST_LDP,
    INST_STR,
    INST_LDR,

    INST_CMP,
    INST_BC,//b.cc
    INST_B,//b
    INST_BL,//bl

    INST_MOV,

    INST_ADR,
    INST_ADRP,

    INST_RET,

    INST_NOP,

    INST_SVC,
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

struct DataBuffer {
    void *result;
    uint64_t size;
};

int getCurrentInstCount();

uint64_t getInstBufferSize();

void emitLabel(const char *label);

void relocateBinary(uint64_t dataSectionVaddrOffset, uint64_t alignment);

InstBuffer *getEmittedInstBuffer();

/**
 * INST_ADD INST_SUB INST_MUL div INST_MOD
 */
void binaryOp3(Arm64Inst inst, uint32_t is64Bit, Operand x, Operand a, Operand b, bool bImm);

/**
 * INST_STP INST_LDP, INST_STR INST_LDR
 */
void binaryOpStoreLoad(Arm64Inst inst, uint32_t is64Bit, Operand reg1, Operand reg2, Operand baseReg, int32_t offset);

/**
 * INST_MOV INST_CMP
 */
void binaryOp2(Arm64Inst inst, uint32_t is64Bit, Operand dist, Operand src, bool srcImm);

/**
 * INST_B.cc, INST_B, INST_BL
 */
void binaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, const char *label);

/**
 * inst_adr
 */
void binaryOpAdr(Arm64Inst inst, Operand dist, const char *label);

/**
 * INST_RET
 */
void binaryOpRet(Arm64Inst inst);

void binaryOpNop(Arm64Inst inst);

void binaryOpSvc(Arm64Inst inst, uint32_t sysCallImm);

/**
 * data section
 */
void binaryData(const char *label, void *buffer, const char *type, uint32_t size);

DataBuffer *getEmittedDataBuffer();

#endif //PCC_CC_BIN_ARM64_H
