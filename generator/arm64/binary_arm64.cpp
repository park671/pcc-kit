//
// Created by Park Yu on 2024/9/23.
//
#include <string.h>
#include "binary_arm64.h"
#include "logger.h"
#include "mspace.h"
#include "register_arm64.h"

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
    logd(BIN_TAG, "%s:", label);
    LabelList *labelList = (LabelList *) pccMalloc(BIN_TAG, sizeof(LabelList));
    labelList->index = instCount;
    labelList->label = label;
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
    return -1;
}

Inst realBinaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, int32_t offset);

void relocateBinary(int32_t baseAddr) {
    InstList *p = instListHead;
    while (p != nullptr) {
        if (p->needRelocation) {
            switch (p->relocateType) {
                case RELOCATE_BRANCH: {
                    int labelIndex = getLabelIndex(p->branchRelocateInfo.label);
                    if (labelIndex == -1) {
                        loge(BIN_TAG, "error: unknown label:%s", p->branchRelocateInfo.label);
                    }
                    p->inst = realBinaryOpBranch(
                            p->branchRelocateInfo.inst,
                            p->branchRelocateInfo.branchCondition,
                            baseAddr + ((labelIndex - p->index) * 4)
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
    buffer->size = sizeof(Inst) * instCount;
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
 * INST_ADD INST_SUB INST_MUL div INST_MOD
 * @param inst
 * @param is64Bit 0 or 1
 * @param x
 * @param a
 * @param b
 */
void binaryOp3(Arm64Inst inst, uint32_t is64Bit, Operand x, Operand a, Operand b, bool bImm) {
    uint32_t baseOp;
    switch (inst) {
        case INST_MUL: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support INST_MUL imm");
                break;
            }
            logd(BIN_TAG, "\tINST_MUL %s %s %s", revertRegisterNames[x], revertRegisterNames[a],
                 revertRegisterNames[b]);
            baseOp = 0x1b007c00 | is64Bit << 31;  // 32: 0x1b, 64位: 0x9b
            emitInst(baseOp | x | a << 5 | b << 16); // INST_MUL
            break;
        }
        case INST_ADD: {
            if (bImm) {
                logd(BIN_TAG, "\tadd %s %s #%d", revertRegisterNames[x], revertRegisterNames[a], b);
                uint32_t imm12 = (b & 0xFFF);  // 12bit imm
                baseOp = 0x11000000 | is64Bit << 31;  // 32: 0x11, 64位: 0x91
                emitInst(baseOp | x | a << 5 | imm12 << 10);  // ADD x, a, #imm12
            } else {
                logd(BIN_TAG, "\tadd %s %s %s", revertRegisterNames[x], revertRegisterNames[a], revertRegisterNames[b]);
                baseOp = 0x0b000000 | is64Bit << 31;  // 32: 0x0b, 64位: 0x8b
                emitInst(baseOp | x | a << 5 | b << 16);  // ADD x, a, INST_B
            }
            break;
        }
        case INST_SUB: {
            if (bImm) {
                logd(BIN_TAG, "\tsub %s %s #%d", revertRegisterNames[x], revertRegisterNames[a], b);
                uint32_t imm12 = (b & 0xFFF);
                baseOp = 0x51000000 | is64Bit << 31;  // 32: 0x51, 64位: 0xd1
                emitInst(baseOp | x | a << 5 | imm12 << 10);  // SUB x, a, #imm12
            } else {
                logd(BIN_TAG, "\tsub %s %s %s", revertRegisterNames[x], revertRegisterNames[a], revertRegisterNames[b]);
                baseOp = 0x4b000000 | is64Bit << 31;  // 32: 0x4b, 64位: 0xcb
                emitInst(baseOp | x | a << 5 | b << 16);  // SUB x, a, INST_B
            }
            break;
        }
        case INST_SDIV: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support div imm");
                break;
            }
            logd(BIN_TAG, "\tsdiv %s %s %s", revertRegisterNames[x], revertRegisterNames[a], revertRegisterNames[b]);
            baseOp = 0x1ac00c00 | is64Bit << 31;  // 32位: 0x1a, 64位: 0x9a
            emitInst(baseOp | x | a << 5 | b << 16); // INST_SDIV
            break;
        }
        case INST_MOD: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support div imm (INST_MOD impl use div)");
                break;
            }
            logd(BIN_TAG, "\tINST_MOD %s %s %s", revertRegisterNames[x], revertRegisterNames[a],
                 revertRegisterNames[b]);
            baseOp = 0x1ac00c00 < is64Bit << 31;  // 32位: 0x1a, 64位: 0x9a
            emitInst(baseOp | 30 | a << 5 | b << 16); // INST_SDIV
            baseOp = 0x1b008000 | is64Bit << 31;  // 32位: 0x1b, 64位: 0x9b
            emitInst(baseOp | x | (uint32_t) 30 << 5 | b << 16 | a << 10); // msub
            break;
        }
        case INST_AND: {

            break;
        }
        case INST_OR: {

            break;
        }
        default: {
            loge(BIN_TAG, "unknown op3 inst:%d", inst);
            break;
        }
    }
}

/**
 *
 * @param isSigned
 * @param size 00-8 01-16 10-32 11-64
 * @param dist
 * @param baseReg
 * @param offset
 */
void ldrInteger(int isSigned, uint32_t size, Operand dist, Operand baseReg, uint64_t offset) {
    if (size >= 2)
        isSigned = 0;
    if (!(offset & ~((uint32_t) 0xfff << size)))
        emitInst(0x39400000 | dist | baseReg << 5 | offset << (10 - size) |
                 (uint32_t) !!isSigned << 23 | size << 30); // ldr(*) x(dist),[x(baseReg),#(offset)]
    else if (offset < 256 || -offset <= 256)
        emitInst(0x38400000 | dist | baseReg << 5 | (offset & 511) << 12 |
                 (uint32_t) !!isSigned << 23 | size << 30); // ldur(*) x(dist),[x(baseReg),#(offset)]
    else {
        binaryOp2(INST_MOV, 1, X30, offset, true); // use x30 for offset
        emitInst(0x38206800 | dist | baseReg << 5 | (uint32_t) 30 << 16 |
                 (uint32_t) (!!isSigned + 1) << 22 | size << 30); // ldr(*) x(dist),[x(baseReg),x30]
    }
}

void strInteger(uint32_t size, Operand dist, Operand baseReg, uint64_t offset) {
    if (!(offset & ~((uint32_t) 0xfff << size)))
        emitInst(0x39000000 | dist | baseReg << 5 | offset << (10 - size) | size << 30);
        // str(*) x(dist),[x(baseReg],#(offset)]
    else if (offset < 256 || -offset <= 256)
        emitInst(0x38000000 | dist | baseReg << 5 | (offset & 511) << 12 | size << 30);
        // stur(*) x(dist),[x(baseReg],#(offset)]
    else {
        binaryOp2(INST_MOV, 1, X30, offset, true);  // use x30 for offset
        emitInst(0x38206800 | dist | baseReg << 5 | (uint32_t) 30 << 16 | size << 30);
        // str(*) x(dist),[x(baseReg),x30]
    }
}

void ldpInteger(uint32_t is64Bit, Operand reg1, Operand reg2, Operand baseReg, int64_t offset) {
    int size = is64Bit ? 0b11 : 0b10;
    int dataSize = 1 << size; // 数据大小，单位为字节
    // 1. 对齐检查
    if ((offset & (dataSize - 1)) != 0) {
        loge(BIN_TAG, "unaligned offset in stp instruction");
        return;
    }
    // 2. 右移偏移量以得到编码所需的偏移量
    int64_t scaledOffset = offset / dataSize;
    if (scaledOffset >= -64 && scaledOffset <= 63) {
        emitInst((0x29400000 | (is64Bit << 31)) | reg1 | (baseReg << 5) | (reg2 << 10) | ((scaledOffset & 0x7F) << 15));
        // 对应于指令：ldp w(reg1), w(reg2), [x(baseReg), #(offset)]
    } else {
        // 处理偏移量超出范围的情况
        // 方法一：使用临时寄存器计算新的基址
        Operand tempReg = X30; // 假设有一个可用的临时寄存器 X_TMP
        // 将新的基址计算到 tempReg 中
        binaryOp3(INST_ADD, 1, tempReg, baseReg, offset, true);
        // 使用偏移量为 0 的 LDP 指令，从 tempReg 加载数据
        emitInst((0x29400000 | (is64Bit << 31)) | reg1 | (tempReg << 5) | (reg2 << 10));
        // 对应于指令：ldp w(reg1), w(reg2), [x(tempReg)]
    }
}

void stpInteger(uint32_t is64Bit, Operand reg1, Operand reg2, Operand baseReg, int64_t offset) {
    int size = is64Bit ? 0b11 : 0b10;
    int dataSize = 1 << size; // 数据大小，单位为字节
    // 1. 对齐检查
    if ((offset & (dataSize - 1)) != 0) {
        loge(BIN_TAG, "unaligned offset in stp instruction");
        return;
    }
    // 2. 计算缩放后的偏移量
    int64_t scaledOffset = offset / dataSize;
    if (scaledOffset >= -64 && scaledOffset <= 63) {
        // 生成指令
        uint32_t opcode =
                (0x29000000 | (is64Bit << 31)) | reg1 | (baseReg << 5) | (reg2 << 10) | ((scaledOffset & 0x7F) << 15);
        emitInst(opcode);
        // 对应于指令：stp [x(baseReg), #(offset)], reg1, reg2
    } else {
        // 处理偏移量超出范围的情况
        // 使用临时寄存器计算新的基址
        Operand tempReg = X30; // 假设有一个可用的临时寄存器 X_TMP
        // 将新的基址计算到 tempReg 中
        binaryOp3(INST_ADD, 1, tempReg, baseReg, offset, true);
        // 使用偏移量为 0 的 STP 指令，向 tempReg 存储数据
        uint32_t opcode = (0x29000000 | (is64Bit << 31)) | reg1 | (tempReg << 5) | (reg2 << 10);
        emitInst(opcode);
        // 对应于指令：stp [x(tempReg)], reg1, reg2
    }
}


/**
 * INST_STP,INST_LDP / INST_STR,INST_LDR
 * @param halfWidth true-32bit false-64bit
 * @param reg1
 * @param reg2 take no effect when "INST_STR,INST_LDR"
 * @param baseReg usually it should be "SP"
 * @param offset [SP, #16]'s #16
 */
void binaryOpStoreLoad(Arm64Inst inst, uint32_t is64Bit, Operand reg1, Operand reg2, Operand baseReg, int32_t offset) {
    switch (inst) {
        case INST_LDP: {
            logd(BIN_TAG, "\tldp %s %s [%s,#%d]", revertRegisterNames[reg1], revertRegisterNames[reg2],
                 revertRegisterNames[baseReg], offset);
            break;
        }
        case INST_STP: {
            logd(BIN_TAG, "\tstp %s %s [%s,#%d]", revertRegisterNames[reg1], revertRegisterNames[reg2],
                 revertRegisterNames[baseReg], offset);
            break;
        }
        case INST_LDR: {
            logd(BIN_TAG, "\tldr %s [%s,#%d]", revertRegisterNames[reg1], revertRegisterNames[baseReg], offset);
            break;
        }
        case INST_STR: {
            logd(BIN_TAG, "\tstr %s [%s,#%d]", revertRegisterNames[reg1], revertRegisterNames[baseReg], offset);
            break;
        }
    }

    int size = is64Bit ? 0b11 : 0b10;
    if (inst == INST_LDP || inst == INST_STP) {
        //double reg
        if (inst == INST_LDP) {
            ldpInteger(is64Bit, reg1, reg2, baseReg, offset);
        } else if (inst == INST_STP) {
            stpInteger(is64Bit, reg1, reg2, baseReg, offset);
        }
    } else if (inst == INST_LDR || inst == INST_STR) {
        //single reg
        if (inst == INST_LDR) {
            ldrInteger(1, size, reg1, baseReg, offset);
        } else if (inst == INST_STR) {
            strInteger(size, reg1, baseReg, offset);
        }
    } else {
        loge(BIN_TAG, "unknown opsl inst:%d", inst);
        return;
    }
}


void movInteger(Operand reg, uint64_t imm) {
    uint64_t mark = 0xffff;//use for check imm size
    int e;
    if (!(imm & ~mark)) {
        //imm is 16bit (high 48bit is 0)
        emitInst(0x52800000 | reg | imm << 5); // movz w(reg),#(imm)
        return;
    }
    if (!(imm & ~(mark << 16))) {
        emitInst(0x52a00000 | reg | imm >> 11); // movz w(reg),#(imm >> 16),lsl #16
        return;
    }
    loge(BIN_TAG, "big imm (>16bit) need .data impl");
}

void movRegister(uint32_t is64bit, Operand dist, Operand src) {
    if (!is64bit) {
        // 32 位操作，使用 ORR 指令
        emitInst(0x2A000000 | dist | (31 << 5) | (src << 16));
        // 等效于：ORR Wd, WZR, Wn
    } else {
        // 64 位操作，使用 ORR 指令
        emitInst(0xAA000000 | dist | (31 << 5) | (src << 16));
        // 等效于：ORR Xd, XZR, Xn
    }
}

void cmpInteger(uint32_t is64bit, Operand reg, uint32_t imm) {
    uint32_t opcode;
    int shift = 0;
    // 检查立即数是否需要移位
    if (imm > 0xFFF) {
        if ((imm & 0xFFF000) == 0) {
            imm >>= 12;
            shift = 1; // 表示立即数左移12位
        } else {
            // 立即数超出范围，需要其他方法处理
            loge(BIN_TAG, "immediate value out of range for cmp instruction");
            return;
        }
    }
    if (!is64bit) {
        // 32 位操作
        opcode = 0x71000000 | (shift << 22) | (imm << 10) | (reg << 5) | 31;
    } else {
        // 64 位操作
        opcode = 0xF1000000 | (shift << 22) | (imm << 10) | (reg << 5) | 31;
    }
    emitInst(opcode);
}

void cmpRegister(uint32_t is64bit, int reg1, int reg2) {
    uint32_t opcode;
    if (!is64bit) {
        // 32 位操作
        opcode = 0x6B000000;
    } else {
        // 64 位操作
        opcode = 0xEB000000;
    }
    opcode = opcode | (reg2 << 16) | (reg1 << 5) | 31;
    emitInst(opcode);
}


/**
 * INST_MOV INST_CMP
 * @param inst
 * @param halfWidth
 * @param dist
 * @param src
 * @param srcImm
 */
void binaryOp2(Arm64Inst inst, uint32_t is64Bit, Operand dist, Operand src, bool srcImm) {
    uint32_t opcode;
    switch (inst) {
        case INST_MOV: {
            if (srcImm) {
                logd(BIN_TAG, "\tmov(imm) %s #%d", revertRegisterNames[dist], src);
                //imm
                movInteger(dist, src);
            } else {
                logd(BIN_TAG, "\tmov(reg) %s %s", revertRegisterNames[dist], revertRegisterNames[src]);
                //move reg to reg
                //arm64 do not have a inst "INST_MOV", use "orr" inst impl
                movRegister(is64Bit, dist, src);
            }
            break;
        }
        case INST_CMP: {
            if (srcImm) {
                logd(BIN_TAG, "\tcmp %s #%d", revertRegisterNames[dist], src);
                uint32_t imm = src;
                cmpInteger(is64Bit, dist, imm);
            } else {
                logd(BIN_TAG, "\tcmp %s %s", revertRegisterNames[dist], revertRegisterNames[src]);
                cmpRegister(is64Bit, dist, src);
            }
            break;
        }
        default: {
            loge(BIN_TAG, "unknown op2 inst:%d", inst);
            return;
        }
    }
}

Inst realBinaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, int32_t offset) {
    Inst instruction = 0;
    switch (inst) {
        case INST_BC: {
            int32_t imm19 = offset >> 2;
            if (imm19 < -(1 << 18) || imm19 >= (1 << 18)) {
                loge(BIN_TAG, "Error: Immediate value out of range for conditional branch.\n");
                return 0;
            }
            uint32_t opcode = 0b01010100 << 24;
            instruction = opcode | ((imm19 & 0x7FFFF) << 5) | (branchCondition & 0x1F);
            break;
        }
        case INST_B: {
            int32_t imm26 = offset >> 2;
            if (imm26 < -(1 << 25) || imm26 >= (1 << 25)) {
                loge(BIN_TAG, "Error: Immediate value out of range for unconditional branch.\n");
                return 0;
            }
            uint32_t opcode = 0b000101 << 26;
            instruction = opcode | (imm26 & 0x03FFFFFF);
            break;
        }
        case INST_BL: {
            int32_t imm26 = offset >> 2;
            if (imm26 < -(1 << 25) || imm26 >= (1 << 25)) {
                loge(BIN_TAG, "Error: Immediate value out of range for branch with link.\n");
                return 0;
            }
            uint32_t opcode = 0b100101 << 26;
            instruction = opcode | (imm26 & 0x03FFFFFF);
            break;
        }
        default:
            loge(BIN_TAG, "Error: Unknown instruction type.\n");
            return 0;
    }
    return instruction;
}

void binaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, const char *label) {
    const char *condString = nullptr;
    switch (branchCondition) {
        case EQ:
            condString = "eq";
            break;
        case NE:
            condString = "ne";
            break;
        case LT:
            condString = "lt";
            break;
        case LE:
            condString = "le";
            break;
        case GT:
            condString = "gt";
            break;
        case GE:
            condString = "ge";
            break;
        case CS:
            condString = "cs";
            break;
        case CC:
            condString = "cc";
            break;
        case VS:
            condString = "vs";
            break;
        case VC:
            condString = "vc";
            break;
        case UNUSED:
            condString = "unused";
            break;
    }
    switch (inst) {
        case INST_BC: {
            logd(BIN_TAG, "\tb.<%s> %s", condString, label);
            break;
        }
        case INST_B: {
            logd(BIN_TAG, "\tb %s", label);
            break;
        }
        case INST_BL: {
            logd(BIN_TAG, "\tbl %s", label);
            break;
        }
    }
    emitRelocateBranchInst(inst, branchCondition, label);
}

void binaryOpRet(Arm64Inst inst) {
    if (inst != INST_RET) {
        loge(BIN_TAG, "unknown INST_RET inst:%d", inst);
        return;
    }
    logd(BIN_TAG, "\tret");
    Inst opcode = 0xd65f03c0;
    emitInst(opcode);
}