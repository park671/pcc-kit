//
// Created by Park Yu on 2024/9/23.
//
#include <string.h>
#include "binary_arm64.h"
#include "logger.h"
#include "mspace.h"
#include "file.h"
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

struct InstDataRelocateInfo {
    Arm64Inst inst;//adrp
    Operand dist;
    const char *label;
};

enum InstRelocateType {
    RELOCATE_BRANCH,
    RELOCATE_DATA,
    RELOCATE_DATA_FIX,
};

struct InstList {
    Inst inst;
    //relocation info
    bool needRelocation;
    int index;
    InstRelocateType relocateType;
    union {
        InstBranchRelocateInfo branchRelocateInfo;
        InstDataRelocateInfo dataRelocateInfo;
    };

    InstList *next;
};

static InstList *instListHead;
static int instCount = 0;

int getCurrentInstCount() {
    return instCount;
}

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

void emitRelocateDataInst(Arm64Inst inst, Operand dist, const char *label) {
    InstList *instList = (InstList *) pccMalloc(BIN_TAG, sizeof(InstList));
    instList->inst = 0;

    instList->needRelocation = true;
    instList->relocateType = RELOCATE_DATA;
    instList->dataRelocateInfo.inst = inst;
    instList->dataRelocateInfo.dist = dist;
    instList->dataRelocateInfo.label = label;
    instList->index = instCount++;

    InstList *fixAddList = nullptr;
    if (inst == INST_ADRP) {
        fixAddList = (InstList *) pccMalloc(BIN_TAG, sizeof(InstList));
        fixAddList->inst = 0;
        fixAddList->needRelocation = true;
        fixAddList->relocateType = RELOCATE_DATA_FIX;
        fixAddList->index = instCount++;
        fixAddList->next = nullptr;
    }

    instList->next = fixAddList;
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

uint64_t getDataOffset(const char *label);

Inst realBinaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, uint64_t offset);

Inst realBinaryOpAdr(Arm64Inst inst, Operand dist, uint64_t offset);

Inst realBinaryAdd(uint32_t is64Bit, Operand x, Operand a, Operand b, bool bImm);

Inst realBinarySub(uint32_t is64Bit, Operand x, Operand a, Operand b, bool bImm);

static volatile bool relocated = false;

uint64_t getInstBufferSize() {
    return getCurrentInstCount() * sizeof(Inst);
}

void relocateBinary(uint64_t dataSectionVaddrOffset, uint64_t alignment) {
    logd(BIN_TAG, "arm64 target relocation...");
    InstList *p = instListHead;
    while (p != nullptr) {
        if (p->needRelocation) {
            switch (p->relocateType) {
                case RELOCATE_BRANCH: {
                    int labelIndex = getLabelIndex(p->branchRelocateInfo.label);
                    if (labelIndex == -1) {
                        loge(BIN_TAG, "error: unknown label:%s", p->branchRelocateInfo.label);
                        exit(-1);
                    }
                    uint64_t textLabelOffset = (labelIndex - p->index) * 4;
                    p->inst = realBinaryOpBranch(
                            p->branchRelocateInfo.inst,
                            p->branchRelocateInfo.branchCondition,
                            textLabelOffset
                    );
                    p->needRelocation = false;
                    break;
                }
                case RELOCATE_DATA: {
                    uint64_t dataSectionInnerOffset = getDataOffset(p->dataRelocateInfo.label);
                    if (dataSectionInnerOffset == INTMAX_MAX) {
                        loge(BIN_TAG, "data not found when relocation");
                        exit(-1);
                    }
                    if (p->dataRelocateInfo.inst == INST_ADR) {
                        uint64_t dataLabelOffset = (dataSectionVaddrOffset + dataSectionInnerOffset) - (p->index * 4);
                        p->inst = realBinaryOpAdr(
                                p->dataRelocateInfo.inst,
                                p->dataRelocateInfo.dist,
                                dataLabelOffset
                        );
                    } else if (p->dataRelocateInfo.inst == INST_ADRP) {
                        uint64_t pcOffset = p->index * 4;
                        uint64_t pcPageOffset = pcOffset & ~0xFFF;
                        uint64_t dataOffset = dataSectionVaddrOffset + dataSectionInnerOffset;
                        uint64_t dataPageOffset = dataOffset & ~0xFFF;
                        uint64_t pageDiff = dataPageOffset - pcPageOffset;
                        p->inst = realBinaryOpAdr(
                                p->dataRelocateInfo.inst,
                                p->dataRelocateInfo.dist,
                                pageDiff >> 12
                        );
                        int64_t immLow = (((int64_t) dataOffset) - ((int64_t) dataPageOffset));
                        if (immLow < 0) {
                            loge(BIN_TAG, "negative offset low imm add inst after adrp!");
                            exit(-1);
                        }
                        //add immLow to adrp result
                        InstList *fixInst = p->next;
                        if (fixInst == nullptr || !fixInst->needRelocation) {
                            loge(BIN_TAG, "no offset low imm add inst after adrp!");
                            exit(-1);
                        }
                        fixInst->inst = realBinaryAdd(true,
                                                      p->dataRelocateInfo.dist,
                                                      p->dataRelocateInfo.dist,
                                                      immLow,
                                                      true);
                        fixInst->needRelocation = false;
                    }
                    p->needRelocation = false;
                    break;
                }
                default: {
                    loge(BIN_TAG, "unknown relocation type:%d", p->relocateType);
                    exit(-1);
                }
            }
        }
        p = p->next;
    }
    relocated = true;
}

InstBuffer *getEmittedInstBuffer() {
    if (!relocated) {
        loge(BIN_TAG, "not relocated yet!");
        exit(-1);
    }
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

Inst realBinaryAdd(uint32_t is64Bit, Operand x, Operand a, Operand b, bool bImm) {
    Inst inst;
    if (bImm) {
        if (b < 0) {
            return realBinarySub(is64Bit, x, a, -b, bImm);
        }
        logd(BIN_TAG, "\tadd %s, %s, #%d", revertRegisterNames[x], revertRegisterNames[a], b);
        uint32_t imm12 = (b & 0xFFF);  // 12bit imm
        inst = 0x11000000 | is64Bit << 31;  // 32: 0x11, 64位: 0x91
        inst = inst | x | a << 5 | imm12 << 10;// ADD x, a, #imm12
    } else {
        logd(BIN_TAG, "\tadd %s, %s, %s", revertRegisterNames[x], revertRegisterNames[a],
             revertRegisterNames[b]);
        inst = 0x0b000000 | is64Bit << 31;  // 32: 0x0b, 64位: 0x8b
        inst = inst | x | a << 5 | b << 16;  // ADD x, a, b
    }
    return inst;
}

Inst realBinarySub(uint32_t is64Bit, Operand x, Operand a, Operand b, bool bImm) {
    Inst inst;
    if (bImm) {
        if (b < 0) {
            return realBinaryAdd(is64Bit, x, a, -b, bImm);
        }
        logd(BIN_TAG, "\tsub %s, %s, #%d", revertRegisterNames[x], revertRegisterNames[a], b);
        uint32_t imm12 = (b & 0xFFF);
        inst = 0x51000000 | is64Bit << 31;  // 32: 0x51, 64位: 0xd1
        inst = inst | x | a << 5 | imm12 << 10;  // SUB x, a, #imm12
    } else {
        logd(BIN_TAG, "\tsub %s, %s, %s", revertRegisterNames[x], revertRegisterNames[a],
             revertRegisterNames[b]);
        inst = 0x4b000000 | is64Bit << 31;  // 32: 0x4b, 64位: 0xcb
        inst = inst | x | a << 5 | b << 16;  // SUB x, a, b
    }
    return inst;
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
            logd(BIN_TAG, "\tmul %s, %s, %s", revertRegisterNames[x], revertRegisterNames[a],
                 revertRegisterNames[b]);
            baseOp = 0x1b007c00 | is64Bit << 31;  // 32: 0x1b, 64位: 0x9b
            emitInst(baseOp | x | a << 5 | b << 16); // INST_MUL
            break;
        }
        case INST_ADD: {
            emitInst(realBinaryAdd(is64Bit, x, a, b, bImm));
            break;
        }
        case INST_SUB: {

            break;
        }
        case INST_SDIV: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support div imm");
                break;
            }
            logd(BIN_TAG, "\tsdiv %s, %s, %s", revertRegisterNames[x], revertRegisterNames[a], revertRegisterNames[b]);
            baseOp = 0x1ac00c00 | is64Bit << 31;  // 32位: 0x1a, 64位: 0x9a
            emitInst(baseOp | x | a << 5 | b << 16); // INST_SDIV
            break;
        }
        case INST_MOD: {
            if (bImm) {
                loge(BIN_TAG, "arm64 not support div imm (INST_MOD impl use div)");
                break;
            }
            logd(BIN_TAG, "\tmod %s, %s, %s", revertRegisterNames[x], revertRegisterNames[a],
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
 * need scale!!!
 * 64bit offset/8
 * 32bit offset/4
 * @param isSigned
 * @param size 00-8 01-16 10-32 11-64
 * @param dist
 * @param baseReg
 * @param offset
 */
void ldrInteger(uint32_t is64Bit, Operand dist, Operand baseReg, uint64_t offset) {
    //do offset align check
    if (is64Bit) {
        if (offset % 8) {
            loge(BIN_TAG, "ldr imm scaled: 64bit offset not 8byte align!");
            exit(-1);
        }
    } else {
        if (offset % 4) {
            loge(BIN_TAG, "ldr imm scaled: 32bit offset not 4byte align!");
            exit(-1);
        }
    }
    offset = is64Bit ? offset / 8 : offset / 4;
    //ldr imm scaled
    uint32_t opCode = 0xB9400000;
    opCode |= is64Bit << 30;
    opCode |= dist;
    opCode |= baseReg << 5;
    opCode |= offset << 10;
    emitInst(opCode);
}

void strInteger(uint32_t is64Bit, Operand dist, Operand baseReg, uint64_t offset) {
    //do offset align check
    if (is64Bit) {
        if (offset % 8) {
            loge(BIN_TAG, "str imm scaled: 64bit offset not 8byte align!");
            exit(-1);
        }
    } else {
        if (offset % 4) {
            loge(BIN_TAG, "str imm scaled: 32bit offset not 4byte align!");
            exit(-1);
        }
    }
    offset = is64Bit ? offset / 8 : offset / 4;
    //str imm scaled
    uint32_t opCode = 0xB9000000;
    opCode |= is64Bit << 30;
    opCode |= dist;
    opCode |= baseReg << 5;
    opCode |= offset << 10;
    emitInst(opCode);
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
            logd(BIN_TAG, "\tldp %s, %s, [%s, #%d]", revertRegisterNames[reg1], revertRegisterNames[reg2],
                 revertRegisterNames[baseReg], offset);
            break;
        }
        case INST_STP: {
            logd(BIN_TAG, "\tstp %s, %s, [%s, #%d]", revertRegisterNames[reg1], revertRegisterNames[reg2],
                 revertRegisterNames[baseReg], offset);
            break;
        }
        case INST_LDR: {
            logd(BIN_TAG, "\tldr %s, [%s, #%d]", revertRegisterNames[reg1], revertRegisterNames[baseReg], offset);
            break;
        }
        case INST_STR: {
            logd(BIN_TAG, "\tstr %s, [%s, #%d]", revertRegisterNames[reg1], revertRegisterNames[baseReg], offset);
            break;
        }
    }

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
            ldrInteger(is64Bit, reg1, baseReg, offset);
        } else if (inst == INST_STR) {
            strInteger(is64Bit, reg1, baseReg, offset);
        }
    } else {
        loge(BIN_TAG, "unknown opsl inst:%d", inst);
        return;
    }
}


void movInteger(Operand reg, int64_t imm) {
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
    if (imm == -1) {
        emitInst(0x92800000);// movn x0 (~#0); ~#0 == 0xFFFF... = #-1
        //fixme: this is a special case.
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
                logd(BIN_TAG, "\tmov %s, #%d", revertRegisterNames[dist], src);
                //imm
                movInteger(dist, src);
            } else {
                logd(BIN_TAG, "\tmov %s, %s", revertRegisterNames[dist], revertRegisterNames[src]);
                //move reg to reg
                //arm64 do not have a inst "INST_MOV", use "orr" inst impl
                movRegister(is64Bit, dist, src);
            }
            break;
        }
        case INST_CMP: {
            if (srcImm) {
                logd(BIN_TAG, "\tcmp %s, #%d", revertRegisterNames[dist], src);
                uint32_t imm = src;
                cmpInteger(is64Bit, dist, imm);
            } else {
                logd(BIN_TAG, "\tcmp %s, %s", revertRegisterNames[dist], revertRegisterNames[src]);
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

Inst realBinaryOpBranch(Arm64Inst inst, BranchCondition branchCondition, uint64_t offset) {
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

Inst realBinaryOpAdr(Arm64Inst inst, Operand dist, uint64_t offset) {
    Inst instruction = 0;
    switch (inst) {
        case INST_ADR: {
            uint64_t offsetLow = offset & 0x3;
            uint64_t offsetHigh = offset >> 2;
            instruction = 0x10000000;
            instruction |= dist;
            instruction |= offsetLow << 29;
            instruction |= offsetHigh << 5;
            break;
        }
        case INST_ADRP: {
            uint64_t offsetLow = offset & 0x3;
            uint64_t offsetHigh = offset >> 2;
            instruction = 0x90000000;
            instruction |= dist;
            instruction |= offsetLow << 29;
            instruction |= offsetHigh << 5;
            break;
        }
        default: {
            loge(BIN_TAG, "realBinaryOpAdr unknown inst: %d", inst);
            exit(-1);
        }
    }
    return instruction;
}

void binaryOpAdr(Arm64Inst inst, Operand dist, const char *label) {
    if (inst == INST_ADR) {
        logd(BIN_TAG, "\tadr %s, %s", revertRegisterNames[dist], label);
        emitRelocateDataInst(inst, dist, label);
    } else if (inst == INST_ADRP) {
        logd(BIN_TAG, "\tadrp %s, %s", revertRegisterNames[dist], label);
        emitRelocateDataInst(inst, dist, label);
    } else {
        loge(BIN_TAG, "unknown address inst: %d", inst);
        exit(-1);
    }
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

void binaryOpNop(Arm64Inst inst) {
    if (inst != INST_NOP) {
        loge(BIN_TAG, "unknown INST_RET inst:%d", inst);
        return;
    }
    logd(BIN_TAG, "\tnop");
    Inst opCode = 0xd503201f;
    emitInst(opCode);
}

/**
 *
 * @param inst
 * @param sysCallImm ignore on linux, must be set on windows!
 */
void binaryOpSvc(Arm64Inst inst, uint32_t sysCallImm) {
    if (inst != INST_SVC) {
        loge(BIN_TAG, "unknown INST_RET inst:%d", inst);
        return;
    }
    logd(BIN_TAG, "\tsvc #%d", sysCallImm);
    Inst opCode = 0xD4000001;
    emitInst(opCode | sysCallImm << 5);
}

struct DataList {
    const char *label;
    void *buffer;
    uint64_t offset;//offset to .data start(vaddr), no alignment
    uint32_t size;
    DataList *next;
};

DataList *dataListHead = nullptr;
uint64_t dataOffset = 0;

uint64_t getDataOffset(const char *label) {
    DataList *p = dataListHead;
    while (p != nullptr) {
        if (strcmp(p->label, label) == 0) {
            return p->offset;
        }
        p = p->next;
    }
    return INTMAX_MAX;
}

void binaryData(const char *label, void *buffer, const char *type, uint32_t size) {
    logd(BIN_TAG, "%s:", label);
    logd(BIN_TAG, "\t#%02X: %s[%d]", dataOffset, type, size);
    DataList *dataList = (DataList *) pccMalloc(BIN_TAG, sizeof(DataList));
    dataList->next = nullptr;
    dataList->label = label;
    dataList->buffer = buffer;
    dataList->size = size;
    dataList->offset = dataOffset;
    dataOffset += size;

    if (dataListHead == nullptr) {
        dataListHead = dataList;
    } else {
        DataList *p = dataListHead;
        while (p->next != nullptr) {
            p = p->next;
        }
        p->next = dataList;
    }
}

DataBuffer *getEmittedDataBuffer() {
    if (!relocated) {
        loge(BIN_TAG, "not relocated yet!");
        exit(-1);
    }
    DataBuffer *dataBuffer = (DataBuffer *) pccMalloc(BIN_TAG, sizeof(DataBuffer));
    dataBuffer->result = pccMalloc(BIN_TAG, dataOffset);
    dataBuffer->size = 0;

    DataList *p = dataListHead;
    uint64_t currentOffset = 0;
    while (p != nullptr) {
        memcpy(((uint8_t *) dataBuffer->result) + currentOffset, p->buffer, p->size);
        currentOffset += p->size;
        dataBuffer->size += p->size;
        p = p->next;
    }
    if (dataBuffer->size != dataOffset) {
        loge(BIN_TAG, "emitted data size error:%d, %d", dataOffset, dataBuffer->size);
        exit(-1);
    }
    return dataBuffer;
}