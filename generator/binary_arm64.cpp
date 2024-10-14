//
// Created by Park Yu on 2024/9/23.
//
#include <string.h>
#include "binary_arm64.h"
#include "logger.h"
#include "mspace.h"

#define BIN_TAG "arm64_bin"

struct LabelList {
    const char *label;
    int index;

    LabelList *next;
};

static LabelList *labelListHead;

struct InstBranchRelocateInfo {
    Arm64Inst inst;
    BranchCondition branchCondition;
    const char *label;
};

enum InstRelocateType {
    RELOCATE_BRANCH,
};

struct InstList {
    Inst inst;
    //relocation info
    bool needRelocation;
    int index;
    InstRelocateType relocateType;
    union {
        InstBranchRelocateInfo branchRelocateInfo;
    };

    InstList *next;
};

static InstList *instListHead;
static int instCount = 0;

void emitLabel(const char *label) {
    LabelList *labelList = (LabelList *) pccMalloc(BIN_TAG, sizeof(LabelList));
    labelList->index = instCount;
    if (labelListHead == nullptr) {
        labelListHead = labelList;
    } else {
        LabelList *p = labelListHead;
        while (p->next != nullptr) {
            p = p->next;
        }
        p->next = labelList;
    }
}

void emitInst(Inst inst) {
    InstList *instList = (InstList *) pccMalloc(BIN_TAG, sizeof(InstList));
    instList->inst = inst;
    instList->needRelocation = false;
    instList->next = nullptr;
    instList->index = instCount++;
    if (instListHead == nullptr) {
        instListHead = instList;
    } else {
        InstList *p = instListHead;
        while (p->next != nullptr) {
            p = p->next;
        }
        p->next = instList;
    }
}

void emitRelocateBranchInst(Arm64Inst inst, BranchCondition branchCondition, const char *label) {
    InstList *instList = (InstList *) pccMalloc(BIN_TAG, sizeof(InstList));
    instList->inst = 0;

    instList->needRelocation = true;
    instList->relocateType = RELOCATE_BRANCH;
    instList->branchRelocateInfo.inst = inst;
    instList->branchRelocateInfo.branchCondition = branchCondition;
    instList->branchRelocateInfo.label = label;

    instList->next = nullptr;
    instList->index = instCount++;
    if (instListHead == nullptr) {
        instListHead = instList;
    } else {
        InstList *p = instListHead;
        while (p->next != nullptr) {
            p = p->next;
        }
        p->next = instList;
    }
}

int getLabelIndex(const char *label) {
    LabelList *p = labelListHead;
    while (p != nullptr) {
        if (p->label != nullptr && strcmp(label, p->label) == 0) {
            return p->index;
        }
        p = p->next;
    }
    return 0;
}

Inst realBinaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, int32_t offset);

void relocate(int32_t baseAddr) {
    InstList *p = instListHead;
    while (p != nullptr) {
        if (p->needRelocation) {
            switch (p->relocateType) {
                case RELOCATE_BRANCH: {
                    int labelIndex = getLabelIndex(p->branchRelocateInfo.label);
                    p->inst = realBinaryOpBranch(
                            p->branchRelocateInfo.inst,
                            p->branchRelocateInfo.branchCondition,
                            baseAddr + (labelIndex * 4)
                    );
                    p->needRelocation = false;
                    break;
                }
                default: {
                    loge(BIN_TAG, "unknown relocation type:%d", p->relocateType);
                }
            }
        }
        p = p->next;
    }
}

InstBuffer *getEmittedInstBuffer() {
    if (instListHead == nullptr) {
        return nullptr;
    }
    InstBuffer *buffer = (InstBuffer *) pccMalloc(BIN_TAG, sizeof(InstBuffer));
    buffer->result = (Inst *) pccMalloc(BIN_TAG, sizeof(Inst) * instCount);
    buffer->size = instCount;
    InstList *p = instListHead;
    int index = 0;
    while (p != nullptr) {
        if (p->needRelocation) {
            loge(BIN_TAG, "error: inst need relocate! #%d: %d", index, p->inst);
        }
        buffer->result[index++] = p->inst;
        InstList *pre = p;
        p = p->next;
        free(pre);
    }
    if (index != instCount) {
        loge(BIN_TAG, "error: inst index invalid! %d, %d", instCount, index);
    }
    instListHead = nullptr;
    instCount = 0;
    return buffer;
}

/**
 * add sub mul div mod
 * @param inst
 * @param halfWidth
 * @param x
 * @param a
 * @param b
 */
void binaryOp3(Arm64Inst inst, bool halfWidth, Operand x, Operand a, Operand b, bool bImm) {
    uint32_t baseOp;
    switch (inst) {
        case mul: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support mul imm");
                break;
            }
            baseOp = halfWidth ? 0x1b007c00 : 0x9b007c00;  // 32: 0x1b, 64位: 0x9b
            emitInst(baseOp | x | a << 5 | b << 16); // mul
            break;
        }
        case add: {
            if (bImm) {
                uint32_t imm12 = (b & 0xFFF);  // 12bit imm
                baseOp = halfWidth ? 0x11000000 : 0x91000000;  // 32: 0x11, 64位: 0x91
                emitInst(baseOp | x | a << 5 | imm12 << 10);  // ADD x, a, #imm12
            } else {
                baseOp = halfWidth ? 0x0b000000 : 0x8b000000;  // 32: 0x0b, 64位: 0x8b
                emitInst(baseOp | x | a << 5 | b << 16);  // ADD x, a, b
            }
            break;
        }
        case sub: {
            if (bImm) {
                uint32_t imm12 = (b & 0xFFF);
                baseOp = halfWidth ? 0x51000000 : 0xd1000000;  // 32: 0x51, 64位: 0xd1
                emitInst(baseOp | x | a << 5 | imm12 << 10);  // SUB x, a, #imm12
            } else {
                baseOp = halfWidth ? 0x4b000000 : 0xcb000000;  // 32: 0x4b, 64位: 0xcb
                emitInst(baseOp | x | a << 5 | b << 16);  // SUB x, a, b
            }
            break;
        }
        case sdiv: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support div imm");
                break;
            }
            baseOp = halfWidth ? 0x1ac00c00 : 0x9ac00c00;  // 32位: 0x1a, 64位: 0x9a
            emitInst(baseOp | x | a << 5 | b << 16); // sdiv
            break;
        }
        case mod: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support div imm (mod impl use div)");
                break;
            }
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
 * stp,ldp / str,ldr
 * @param halfWidth true-32bit false-64bit
 * @param reg1
 * @param reg2 take no effect when "str,ldr"
 * @param baseReg usually it should be "SP"
 * @param offset [SP, #16]'s #16
 */
void binaryOpStoreLoad(Arm64Inst inst, bool halfWidth, Operand reg1, Operand reg2, Operand baseReg, int32_t offset) {
    uint32_t opcode;

    if (inst == ldp || inst == stp) {
        //double reg
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
    } else if (inst == ldr || inst == str) {
        //single reg
        if (inst == ldr) {
            if (halfWidth) {
                opcode = 0xB9400000;  // 32 LDR Wt, [Xn, imm12]
            } else {
                opcode = 0xF9400000;  // 64 LDR Xt, [Xn, imm12]
            }
        } else if (inst == str) {
            if (halfWidth) {
                opcode = 0xB9000000;  // 32 STR Wt, [Xn, imm12]
            } else {
                opcode = 0xF9000000;  // 64 STR Xt, [Xn, imm12]
            }
        }

        // offset must be in the range [0, 4095]
        if (offset < 0 || offset > 4095) {
            loge(BIN_TAG, "error: offset out of range\n");
            return;
        }

        int imm12 = (offset >> 2) & 0xFFF;  // 12-bit immediate value shifted to be byte-aligned
        opcode |= (imm12 << 10);  // imm12 in bits [21:10]
        opcode |= (baseReg << 5); // base register in bits [9:5]
        opcode |= reg1;            // target register in bits [4:0]
    } else {
        loge(BIN_TAG, "unknown opsl inst:%d", inst);
        return;
    }

    emitInst(opcode);
}

/**
 * mov cmp
 * @param inst
 * @param halfWidth
 * @param dest
 * @param src
 * @param srcImm
 */
void binaryOp2(Arm64Inst inst, bool halfWidth, Operand dest, Operand src, bool srcImm) {
    uint32_t opcode;
    switch (inst) {
        case mov: {
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
            break;
        }
        case cmp: {
            if (srcImm) {
                uint32_t imm = src;
                if (halfWidth) {
                    // 32 CMP: SUBS WZR, Wn, #imm
                    opcode = 0x71000000 | (dest << 5) | imm;  // 32-bit SUBS WZR, Wn, imm
                } else {
                    // 64 CMP: SUBS XZR, Xn, #imm
                    opcode = 0xF1000000 | (dest << 5) | imm;  // 64-bit SUBS XZR, Xn, imm
                }
            } else {
                if (halfWidth) {
                    // 32 CMP: SUBS WZR, Wn, Wm
                    opcode = 0x6B00001F | (src << 16) | (dest << 5);  // 32-bit SUBS WZR, Wn, Wm
                } else {
                    // 64 CMP: SUBS XZR, Xn, Xm
                    opcode = 0xEB00001F | (src << 16) | (dest << 5);  // 64-bit SUBS XZR, Xn, Xm
                }
            }
            break;
        }
        default: {
            loge(BIN_TAG, "unknown op2 inst:%d", inst);
            return;
        }
    }
    emitInst(opcode);
}

Inst realBinaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, int32_t offset) {
    switch (inst) {
        case bc: {
            uint32_t opcode = 0x54000000;
            opcode |= (branchCondition & 0xF) << 24;
            if (offset < -0x100000 || offset > 0xFFFFC) {
                loge(BIN_TAG, "offset out of range\n");
                return 0;
            }
            opcode |= ((offset >> 2) & 0x7FFFF);
            return opcode;
            break;
        }
        case b: {
            uint32_t opcode = 0x14000000;
            if (offset < -0x2000000 || offset > 0x1FFFFFF) {
                loge(BIN_TAG, "offset out of range\n");
                return 0;
            }
            opcode |= ((offset >> 2) & 0x03FFFFFF);
            return opcode;
            break;
        }
        case bl: {
            uint32_t opcode = 0x94000000;
            if (offset < -0x2000000 || offset > 0x1FFFFFF) {
                loge(BIN_TAG, "offset out of range\n");
                return 0;
            }
            opcode |= ((offset >> 2) & 0x03FFFFFF);
            return opcode;
            break;
        }
        default: {
            loge(BIN_TAG, "unknown opb inst:%d", inst);
            return 0;
        }
    }
}

void binaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, const char *label) {
    emitRelocateBranchInst(inst, branchCondition, label);
}

void binaryOpRet(Arm64Inst inst) {
    if (inst != ret) {
        loge(BIN_TAG, "unknown ret inst:%d", inst);
        return;
    }
    Inst opcode = 0xd65f03c0;
    emitInst(opcode);
}