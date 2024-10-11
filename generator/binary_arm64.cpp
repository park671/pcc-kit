//
// Created by Park Yu on 2024/9/23.
//

#include "binary_arm64.h"
#include "elf.h"
#include "file.h"
#include "logger.h"

#define BIN_TAG "arm64_bin"

enum Arm64Inst {
    unknown,

    mul,
    add,
    sub,
    sdiv,
    mod,//fake inst

    stp,
    ldp,

    mov,

    ret
};

typedef uint32_t Operand;
typedef uint32_t Condition;

typedef uint32_t Inst;

void emitInst(Inst inst) {
    writeFileB(&inst, sizeof(Inst));
}

/**
 * add sub mul div mod
 * @param inst
 * @param halfWidth
 * @param x
 * @param a
 * @param b
 */
void generateOp3WithCondition(Arm64Inst inst, bool halfWidth, Operand x, Operand a, Operand b) {
    uint32_t baseOp;
    switch (inst) {
        case mul: {
            baseOp = halfWidth ? 0x1b007c00 : 0x9b007c00;  // 32位: 0x1b, 64位: 0x9b
            emitInst(baseOp | x | a << 5 | b << 16); // mul
            break;
        }
        case add: {
            baseOp = halfWidth ? 0x0b000000 : 0x8b000000;  // 32位: 0x0b, 64位: 0x8b
            emitInst(baseOp | x | a << 5 | b << 16); // add
            break;
        }
        case sub: {
            baseOp = halfWidth ? 0x4b000000 : 0xcb000000;  // 32位: 0x4b, 64位: 0xcb
            emitInst(baseOp | x | a << 5 | b << 16); // sub
            break;
        }
        case sdiv: {
            baseOp = halfWidth ? 0x1ac00c00 : 0x9ac00c00;  // 32位: 0x1a, 64位: 0x9a
            emitInst(baseOp | x | a << 5 | b << 16); // sdiv
            break;
        }
        case mod: {
            baseOp = halfWidth ? 0x1ac00c00 : 0x9ac00c00;  // 32位: 0x1a, 64位: 0x9a
            emitInst(baseOp | 30 | a << 5 | b << 16); // sdiv
            baseOp = halfWidth ? 0x1b008000 : 0x9b008000;  // 32位: 0x1b, 64位: 0x9b
            emitInst(baseOp | x | (uint32_t) 30 << 5 | b << 16 | a << 10); // msub
            break;
        }
        default: {
            loge(BIN_TAG, "unknown op3 inst:%d", inst);
            break;
        }
    }
}

/**
 * stp ldp
 * @param halfWidth true-32bit false-64bit
 * @param reg1
 * @param reg2
 * @param baseReg usually it should be "SP"
 * @param offset [SP, #16]'s #16
 */
void generateOp4(Arm64Inst inst, bool halfWidth, Operand reg1, Operand reg2, Operand baseReg, int offset) {
    uint32_t opcode;
    if (inst == ldp) {
        if (halfWidth) {
            opcode = 0x29400000;  // 32 LDP Wt1, Wt2, [Xn, imm7]
        } else {
            opcode = 0xA9400000;  // 64 LDP Xt1, Xt2, [Xn, imm7]
        }
    } else if (inst == stp) {
        if (halfWidth) {
            opcode = 0x28800000;  // 32 STP Wt1, Wt2, [Xn, imm7]
        } else {
            opcode = 0xA8800000;  // 64 STP Xt1, Xt2, [Xn, imm7]
        }
    } else {
        loge(BIN_TAG, "unknown op4 inst:%d", inst);
        return;
    }

    // offset [-64, 63], must be multi of 8
    if (offset < -64 || offset > 63 || (offset % 8 != 0)) {
        loge(BIN_TAG, "error: offset out of range or not a multiple of 8\n");
        return;
    }

    int imm7 = (offset / 8) & 0x7F;
    opcode |= (imm7 << 15);
    opcode |= (reg2 << 10);
    opcode |= (baseReg << 5);
    opcode |= reg1;
    emitInst(opcode);
}

/**
 * mov
 * @param inst
 * @param halfWidth
 * @param dest
 * @param src
 * @param srcImm
 */
void generateOp2(Arm64Inst inst, bool halfWidth, Operand dest, Operand src, bool srcImm) {
    uint32_t opcode;
    if (inst == mov) {
        if (srcImm) {
            //imm
            uint16_t imm16 = (uint16_t) src;
            if (halfWidth) {
                //32
                opcode = 0x52800000 | (dest & 0x1F) | (imm16 << 5);  // MOVZ Wd, #imm16
            } else {
                //64
                opcode = 0xD2800000 | (dest & 0x1F) | (imm16 << 5);  // MOVZ Xd, #imm16
            }
        } else {
            //move reg to reg
            //arm64 do not have a inst "mov", use "orr" inst impl
            if (halfWidth) {
                //32
                opcode = 0x2A0003E0 | (dest & 0x1F) | ((src & 0x1F) << 5);  // ORR Wd, WZR, Wn
            } else {
                //64
                opcode = 0xAA0003E0 | (dest & 0x1F) | ((src & 0x1F) << 5);  // ORR Xd, XZR, Xn
            }
        }
    } else {
        loge(BIN_TAG, "unknown op2 inst:%d", inst);
        return;
    }
    emitInst(opcode);
}

void generateArm64Binary(const char *assemblyFileName) {


//    writeElf64();
}