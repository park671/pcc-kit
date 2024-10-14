//
// Created by Park Yu on 2024/9/23.
//

#include <stdio.h>
#include <vector>
#include "assembler_arm64.h"
#include "logger.h"
#include "file.h"
#include "register_arm64.h"
#include "binary_arm64.h"

using namespace std;

#define ARM64_TAG "arm64_asm"

#define ARM_STACK_ALIGN 16
#define ARM_BLOCK_32_ALIGN 4
#define ARM_BLOCK_64_ALIGN 8

static int outputAssembly = 0;

int alignBlockSize(int reqSize) {
    if (reqSize > ARM_BLOCK_32_ALIGN) {
        //64bit
        return ARM_BLOCK_64_ALIGN;
    } else {
        //32bit
        return ARM_BLOCK_32_ALIGN;
    }
}

int alignStackSize(int reqSize) {
    if (reqSize < ARM_STACK_ALIGN) {
        return ARM_STACK_ALIGN;
    }
    int blocks = reqSize / ARM_STACK_ALIGN;
    if (reqSize % ARM_STACK_ALIGN != 0) {
        blocks++;
    }
    return blocks * ARM_STACK_ALIGN;
}

static vector<StackVar *> currentStackVarList;

const char *getSection(SectionType sectionType) {
    switch (sectionType) {
        case SECTION_TEXT:
            return ".text";
        case SECTION_DATA:
            return ".data";
        case SECTION_BSS:
            return ".bss";
        case SECTION_RODATA:
            return ".rodata";
    }
}

static vector<int> regInUseList;

static void atomicRegister() {
    regInUseList.clear();
}

char *getCommonRegName(int regIndex, int size) {
    char *regName = (char *) malloc(sizeof(char) * 4);//xnn
    char regWidth = 'x';
    if (size > 4) {
        regWidth = 'x';
    } else {
        regWidth = 'w';
    }
    snprintf(regName, 4, "%c%s", regWidth, commonRegisterName[regIndex]);
    return regName;
}

static char *commonRegsVarName[COMMON_REG_SIZE];

/**
 *
 * @param varName
 * @return offset in common regs array or -1 not found.
 */
int isVarExistInCommonReg(char *varName) {
    for (int i = 0; i < COMMON_REG_SIZE; i++) {
        if (commonRegsVarName[i] != nullptr && strcmp(commonRegsVarName[i], varName) == 0) {
            return i;
        }
    }
    return -1;
}

bool isOperandEqualVarName(MirOperand *mirOperand, char *varName) {
    if (mirOperand == nullptr) {
        return false;
    }
    if (mirOperand->type == OPERAND_IDENTITY) {
        if (strcmp(mirOperand->identity, varName) == 0) {
            return true;
        }
    }
    return false;
}

bool isMirCodeUseVar(MirCode *mirCode, char *varName) {
    switch (mirCode->mirType) {
        case MIR_2: {
            Mir2 *mir2 = mirCode->mir2;
            if (strcmp(mir2->distIdentity, varName) == 0) {
                return true;
            }
            if (isOperandEqualVarName(&mir2->fromValue, varName)) {
                return true;
            }
            break;
        }
        case MIR_3: {
            Mir3 *mir3 = mirCode->mir3;
            if (strcmp(mir3->distIdentity, varName) == 0) {
                return true;
            }
            if (isOperandEqualVarName(&mir3->value1, varName)) {
                return true;
            }
            if (isOperandEqualVarName(&mir3->value2, varName)) {
                return true;
            }
            break;
        }
        case MIR_CMP: {
            MirCmp *mirCmp = mirCode->mirCmp;
            if (isOperandEqualVarName(&mirCmp->value1, varName)) {
                return true;
            }
            if (isOperandEqualVarName(&mirCmp->value2, varName)) {
                return true;
            }
            break;
        }
        case MIR_CALL: {
            MirCall *mirCall = mirCode->mirCall;
            if (mirCall->mirObjectList != nullptr) {
                MirObjectList *objectList = mirCall->mirObjectList;
                while (objectList != nullptr) {
                    if (isOperandEqualVarName(&objectList->value, varName) == 0) {
                        return true;
                    }
                    objectList = objectList->next;
                }
            }
            break;
        }
        case MIR_RET: {
            MirRet *mirRet = mirCode->mirRet;
            if (isOperandEqualVarName(mirRet->value, varName)) {
                return true;
            }
            break;
        }
        case MIR_JMP:
        case MIR_LABEL:
        default: {
            //these cases do not use var.
            break;
        }
    }
    return false;
}

int leastRecentlyUseLine(MirCode *mirCode, char *varName) {
    while (mirCode != nullptr && varName != nullptr) {
        if (isMirCodeUseVar(mirCode, varName)) {
            return mirCode->codeLine;
        }
        mirCode = mirCode->nextCode;
    }
    return INT32_MAX;
}

int usedRegs = 0;

bool isRegAtomicInUse(int bestRegIndex) {
    for (int i = 0; i < regInUseList.size(); i++) {
        if (regInUseList[i] == bestRegIndex) {
            return true;
        }
    }
    return false;
}

void clearRegs() {
    for (int i = 0; i < COMMON_REG_SIZE; i++) {
        commonRegsVarName[i] = nullptr;
    }
    usedRegs = 0;
}

int allocEmptyReg(MirCode *mirCode) {
    if (usedRegs < COMMON_REG_SIZE) {
        return usedRegs++;
    } else {
        int bestRegIndex = -1;
        int maxCodeLine = -1;
        for (int i = 0; i < COMMON_REG_SIZE; i++) {
            if (isRegAtomicInUse(i)) {
                continue;
            }
            int codeLine = leastRecentlyUseLine(mirCode, commonRegsVarName[i]);
            if (codeLine == -1) {
                continue;
            }
            if (maxCodeLine < codeLine) {
                maxCodeLine = codeLine;
                bestRegIndex = i;
            }
            if (codeLine == INT32_MAX) {
                break;
            }
        }
        if (bestRegIndex != -1) {
            regInUseList.push_back(bestRegIndex);
        }
        return bestRegIndex;
    }
}

int allocParamReg(int paramIndex) {
    return paramIndex;//start from x0
}

void generateSection(SectionType sectionType) {
    writeFile(".section %s\n", getSection(sectionType));
}

int currentStackTop = 0;
int stackStep = 4;

void allocStack(int stackSize) {
    currentStackTop = alignStackSize(stackSize);
    //save x29 & x30
    currentStackTop += 16;
    binaryOp3(sub, false, SP, SP, currentStackTop, true);
    writeFile("\tsub\tsp, sp, #%d\n", currentStackTop);
    currentStackTop -= 16;
    binaryOpStoreLoad(stp, false, X29, X30, SP, currentStackTop);
    writeFile("\tstp\tx29, x30, [sp, #%d]\n", currentStackTop);//2 x 64bit = 16byte
    binaryOp3(add, false, X29, SP, currentStackTop, true);
    writeFile("\tadd\tx29, sp, #%d\n", currentStackTop);
    currentStackTop -= stackStep;
}

int allocVarFromStack(const char *varName, int sizeInByte) {
    int size = alignBlockSize(sizeInByte);
    StackVar *stackVar = (StackVar *) malloc(sizeof(StackVar));
    stackVar->varName = varName;
    stackVar->varSize = size;
    stackVar->stackOffset = currentStackTop;
    currentStackVarList.push_back(stackVar);
    int offset = currentStackTop;
    int alignMargin = 0;
    if (size > ARM_BLOCK_32_ALIGN) {
        //8byte align
        while (offset % ARM_BLOCK_64_ALIGN != 0) {
            //not align yet
            alignMargin++;
            offset--;
        }
    } else {
        //4byte align
        while (offset % ARM_BLOCK_32_ALIGN != 0) {
            //not align yet
            alignMargin++;
            offset--;
        }
    }
    currentStackTop -= size + alignMargin;
    return offset;
}

int getVarSizeFromStack(const char *varName) {
    for (int i = 0; i < currentStackVarList.size(); i++) {
        if (strcmp(currentStackVarList[i]->varName, varName) == 0) {
            return currentStackVarList[i]->varSize;
            break;
        }
    }
    loge(ARM64_TAG, "unknown var size %s", varName);
    return -1;
}

/**
 * complete
 * @param mirOperandType
 * @return
 */
int getPrimitiveMirOperandSizeInByte(MirOperandType mirOperandType) {
    switch (mirOperandType) {
        case OPERAND_INT8:
        case OPERAND_INT16:
        case OPERAND_INT32:
            return ARM_BLOCK_32_ALIGN;
        case OPERAND_INT64:
            return ARM_BLOCK_64_ALIGN;
        case OPERAND_FLOAT32:
            return ARM_BLOCK_32_ALIGN;
        case OPERAND_FLOAT64:
            return ARM_BLOCK_64_ALIGN;
        case OPERAND_UNKNOWN:
        case OPERAND_IDENTITY:
        case OPERAND_RET:
        case OPERAND_VOID:
        default: {
            loge(ARM64_TAG, "internal error: invalid type for sizing %d", mirOperandType);
            return 0;
        }
    }
}

int getOperandSize(MirOperand *mirOperand) {
    if (mirOperand->type == OPERAND_IDENTITY) {
        return getVarSizeFromStack(mirOperand->identity);
    } else if (mirOperand->type == OPERAND_RET) {
        return ARM_BLOCK_64_ALIGN;
    } else {
        return getPrimitiveMirOperandSizeInByte(mirOperand->type);
    }
}

/**
 * get var from stack by name
 * @param varName
 * @param sizeInByte
 * @return offset from stack bottom
 */
int getVarFromStack(const char *varName) {
    int offset = -1;
    for (int i = 0; i < currentStackVarList.size(); i++) {
        if (strcmp(currentStackVarList[i]->varName, varName) == 0) {
            offset = currentStackVarList[i]->stackOffset;
            break;
        }
    }
    return offset;
}

void releaseStack(int stackSize) {
    currentStackTop = alignStackSize(stackSize);
    //save x29 & x30
    currentStackTop += 16;
    binaryOpStoreLoad(ldp, false, X29, X30, SP, currentStackTop - 16);
    writeFile("\tldp\tx29, x30, [sp, #%d]\n", currentStackTop - 16);
    binaryOp3(add, false, SP, SP, currentStackTop, true);
    writeFile("\tadd\tsp, sp, #%d\n", currentStackTop);
}

void storeParamsToStack(MirMethodParam *param) {
    //from x0 - x7
    int paramIndex = 0;
    while (param != nullptr) {
        int offset = allocVarFromStack(param->paramName, param->byte);
        const char *regName = getCommonRegName(paramIndex, param->byte);
        binaryOpStoreLoad(
                str,
                false,
                commonRegisterBinary[paramIndex],
                0,
                SP,
                offset);
        writeFile("\tstr\t%s, [sp, #%d]\n", regName, offset);
        free((void *) regName);
        param = param->next;
        paramIndex++;
    }
}

/**
 * complete
 * @param mirOperand
 * @return index of common reg
 */
int loadVarIntoReg(MirCode *mirCode, MirOperand *mirOperand) {
    if (mirOperand->type != OPERAND_IDENTITY) {
        loge(ARM64_TAG, "internal error: get var but not identity type operand");
        return -1;
    }
    int fromRegIndex = isVarExistInCommonReg(mirOperand->identity);
    if (fromRegIndex == -1) {
        int stackOffset = getVarFromStack(mirOperand->identity);
        if (stackOffset == -1) {
            loge(ARM64_TAG, "internal error: can not found var %s on stack", mirOperand->identity);
            return -1;
        }
        fromRegIndex = allocEmptyReg(mirCode);
        const char *fromRegName = getCommonRegName(
                fromRegIndex,
                getVarSizeFromStack(mirOperand->identity)
        );
        binaryOpStoreLoad(
                ldr,
                false,
                commonRegisterBinary[fromRegIndex],
                0,
                SP,
                stackOffset);
        writeFile("\tldr\t%s, [sp, #%d]\n", fromRegName, stackOffset);
        free(((void *) fromRegName));
        commonRegsVarName[fromRegIndex] = mirOperand->identity;
    }
    return fromRegIndex;
}

int convertMirOperandImmBinary(MirOperand *mirOperand) {
    char *result = nullptr;
    switch (mirOperand->type) {
        case OPERAND_INT8:
            return mirOperand->dataInt8;
        case OPERAND_INT16:
            return mirOperand->dataInt16;
        case OPERAND_INT32:
        case OPERAND_INT64:
        case OPERAND_FLOAT32:
        case OPERAND_FLOAT64:
        default: {
            loge(ARM64_TAG, "invalid operand for imm %d", mirOperand->type);
            return 0;
        }
    }
}

const char *convertMirOperandAsm(MirOperand *mirOperand) {
    char *result = nullptr;
    switch (mirOperand->type) {
        case OPERAND_INT8:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "#%d", mirOperand->dataInt8);
            return result;
        case OPERAND_INT16:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "#%d", mirOperand->dataInt16);
            return result;
        case OPERAND_INT32:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "#%d", mirOperand->dataInt32);
            return result;
        case OPERAND_INT64:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "#%lld", mirOperand->dataInt64);
            return result;
        case OPERAND_FLOAT32:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "#%f", mirOperand->dataFloat32);
            return result;
        case OPERAND_FLOAT64:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "#%f", mirOperand->dataFloat64);
            return result;
        default: {
            loge(ARM64_TAG, "invalid operand for convert %d", mirOperand->type);
            return "invalid";
        }
    }
}

void generateCodes(MirCode *mirCode) {
    atomicRegister();
    switch (mirCode->mirType) {
        case MIR_2: {
            Mir2 *mir2 = mirCode->mir2;
            int distRegIndex = isVarExistInCommonReg(mir2->distIdentity);
            if (distRegIndex == -1) {
                distRegIndex = allocEmptyReg(mirCode);
            }
            int greaterRegisterWidth = 0;
            int distRegisterWidth = getPrimitiveMirOperandSizeInByte(mir2->distType);
            int fromRegisterWidth = getOperandSize(&mir2->fromValue);

            greaterRegisterWidth = distRegisterWidth > fromRegisterWidth ? distRegisterWidth : fromRegisterWidth;

            const char *distRegName = getCommonRegName(
                    distRegIndex,
                    greaterRegisterWidth
            );

            MirOperand *mirOperand = &mir2->fromValue;
            if (mirOperand->type == OPERAND_IDENTITY) {
                int fromRegIndex = isVarExistInCommonReg(mirOperand->identity);
                if (fromRegIndex == -1) {
                    int stackOffset = getVarFromStack(mirOperand->identity);
                    if (stackOffset == -1) {
                        loge(ARM64_TAG, "internal error: can not found var %s on stack", mirOperand->identity);
                    }
                    binaryOpStoreLoad(
                            ldr,
                            greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                            distRegIndex,
                            0,
                            SP,
                            stackOffset);
                    writeFile("\tldr\t%s, [sp, #%d]\n", distRegName, stackOffset);
                } else {
                    const char *fromRegName = getCommonRegName(
                            fromRegIndex,
                            greaterRegisterWidth
                    );
                    binaryOp2(mov, greaterRegisterWidth == ARM_BLOCK_32_ALIGN, distRegIndex, fromRegIndex, false);
                    writeFile("\tmov\t%s, %s\n", distRegName, fromRegName);
                    free(((void *) fromRegName));
                }
            } else {
                writeFile("\tmov\t%s, ", distRegName);
                switch (mirOperand->type) {
                    case OPERAND_RET: {
                        //ret must be x0
                        binaryOp2(mov,
                                  greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                                  distRegIndex,
                                  X0,
                                  false
                        );
                        writeFile("%s", getCommonRegName(
                                0,
                                greaterRegisterWidth
                        ));
                        break;
                    }
                    case OPERAND_INT8:
                    case OPERAND_INT16:
                    case OPERAND_INT32: {
                        binaryOp2(mov,
                                  greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                                  distRegIndex,
                                  mirOperand->dataInt32,
                                  true
                        );
                        writeFile("#%d", mirOperand->dataInt32);
                        break;
                    }
                    case OPERAND_INT64:
                    case OPERAND_FLOAT32:
                    case OPERAND_FLOAT64: {
                        loge(ARM64_TAG, "not impl yet(big imm need be on stack)");
                        //todo this is wrong
                        binaryOp2(mov,
                                  greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                                  distRegIndex,
                                  mirOperand->dataFloat64,
                                  true
                        );
                        writeFile("#%f", mirOperand->dataFloat64);
                        break;
                    }
                }
                writeFile("\n");
            }
            commonRegsVarName[distRegIndex] = mir2->distIdentity;
            int distStackOffset = getVarFromStack(mir2->distIdentity);
            if (distStackOffset == -1) {
                int distSize = getPrimitiveMirOperandSizeInByte(mir2->distType);
                distStackOffset = allocVarFromStack(mir2->distIdentity, distSize);
            }
            binaryOpStoreLoad(
                    str,
                    greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                    distRegIndex,
                    0,
                    SP,
                    distStackOffset);
            writeFile("\tstr\t%s, [sp, #%d]\n", distRegName, distStackOffset);
            free(((void *) distRegName));
            break;
        }
        case MIR_3: {
            Mir3 *mir3 = mirCode->mir3;
            //value1
            MirOperand *mirOperand = &mir3->value1;

            //get the greatest width
            int distRegisterWidth = getPrimitiveMirOperandSizeInByte(mir3->distType);
            int value1RegisterWidth = getOperandSize(&mir3->value1);
            int value2RegisterWidth = getOperandSize(&mir3->value2);

            int greaterRegisterWidth =
                    distRegisterWidth > value1RegisterWidth ? distRegisterWidth : value1RegisterWidth;
            greaterRegisterWidth =
                    greaterRegisterWidth > value2RegisterWidth ? greaterRegisterWidth : value2RegisterWidth;


            const char *value1 = nullptr;
            int value1RegIndex = -1;
            if (mirOperand->type == OPERAND_IDENTITY) {
                value1RegIndex = loadVarIntoReg(mirCode, mirOperand);
            } else if (mirOperand->type == OPERAND_RET) {
                value1RegIndex = 0;
            } else {
                //value1 must be reg!!!
                value1RegIndex = allocEmptyReg(mirCode);
                value1 = getCommonRegName(
                        value1RegIndex,
                        greaterRegisterWidth
                );
                binaryOp2(mov,
                          greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                          commonRegisterBinary[value1RegIndex],
                          convertMirOperandImmBinary(mirOperand),
                          true
                );
                writeFile("\tmov\t%s, %s\n", value1, convertMirOperandAsm(mirOperand));
            }
            if (value1 == nullptr) {
                value1 = getCommonRegName(
                        value1RegIndex,
                        greaterRegisterWidth
                );
            }
            commonRegsVarName[value1RegIndex] = "mir3_value1";

            //value2
            int value2RegIndex = -1;
            int value2Imm = 0;
            mirOperand = &mir3->value2;
            const char *value2 = nullptr;
            if (mirOperand->type == OPERAND_IDENTITY) {
                value2RegIndex = loadVarIntoReg(mirCode, mirOperand);
                value2 = getCommonRegName(
                        value2RegIndex,
                        greaterRegisterWidth
                );
                commonRegsVarName[value2RegIndex] = "mir3_value2";
            } else if (mirOperand->type == OPERAND_RET) {
                value2RegIndex =;
                value2 = getCommonRegName(
                        0,
                        ARM_BLOCK_64_ALIGN
                );
                commonRegsVarName[0] = "mir3_value2";
            } else {
                if (mir3->op == OP_ADD || mir3->op == OP_SUB) {
                    //value2 can be a imm
                    value2RegIndex = -1;
                    value2Imm = convertMirOperandImmBinary(mirOperand);
                    value2 = convertMirOperandAsm(mirOperand);
                } else {
                    //stupid arm64 do not support imm in mul & div inst
                    //value2 must be reg!!!
                    value2RegIndex = allocEmptyReg(mirCode);
                    value2 = getCommonRegName(
                            value2RegIndex,
                            greaterRegisterWidth
                    );
                    binaryOp2(mov,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[value2RegIndex],
                              convertMirOperandImmBinary(mirOperand),
                              true
                    );
                    writeFile("\tmov\t%s, %s\n", value2, convertMirOperandAsm(mirOperand));
                    commonRegsVarName[value2RegIndex] = "mir3_value2";
                }
            }
            //dist
            const char *dist = nullptr;
            int distRegIndex = isVarExistInCommonReg(mir3->distIdentity);
            if (distRegIndex == -1) {
                distRegIndex = value1RegIndex;
                dist = getCommonRegName(
                        value1RegIndex,
                        greaterRegisterWidth
                );
            } else {
                dist = getCommonRegName(
                        distRegIndex,
                        greaterRegisterWidth
                );
            }
            Operand b = 0;
            if (value2RegIndex == -1) {
                b = value2Imm;
            } else {
                b = commonRegisterBinary[value2RegIndex];
            }
            //op
            switch (mir3->op) {
                case OP_UNKNOWN:
                    break;
                case OP_ADD:
                    binaryOp3(add,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[distRegIndex],
                              commonRegisterBinary[value1RegIndex],
                              b,
                              value2RegIndex == -1);
                    writeFile("\tadd\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_SUB:
                    binaryOp3(sub,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[distRegIndex],
                              commonRegisterBinary[value1RegIndex],
                              b,
                              value2RegIndex == -1);
                    writeFile("\tsub\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_MUL:
                    binaryOp3(mul,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[distRegIndex],
                              commonRegisterBinary[value1RegIndex],
                              b,
                              value2RegIndex == -1);
                    writeFile("\tmul\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_DIV:
                    binaryOp3(sdiv,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[distRegIndex],
                              commonRegisterBinary[value1RegIndex],
                              b,
                              value2RegIndex == -1);
                    writeFile("\tdiv\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_MOD:
                    binaryOp3(mod,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[distRegIndex],
                              commonRegisterBinary[value1RegIndex],
                              b,
                              value2RegIndex == -1);
                    writeFile("\tmod\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_ASSIGNMENT:
                    break;
            }
            commonRegsVarName[distRegIndex] = mir3->distIdentity;
            int distStackOffset = getVarFromStack(mir3->distIdentity);
            if (distStackOffset == -1) {
                int distSize = getPrimitiveMirOperandSizeInByte(mir3->distType);
                distStackOffset = allocVarFromStack(mir3->distIdentity, distSize);
            }
            binaryOpStoreLoad(str,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[distRegIndex],
                              0,
                              SP,
                              distStackOffset);
            writeFile("\tstr\t%s, [sp, #%d]\n", dist, distStackOffset);
            free(((void *) dist));
            free(((void *) value1));
            free(((void *) value2));
            break;
        }
        case MIR_CALL: {
            MirCall *mirCall = mirCode->mirCall;
            //move the param objs to common regs
            MirObjectList *mirObjectList = mirCall->mirObjectList;
            int paramIndex = 0;
            while (mirObjectList != nullptr) {
                MirOperand *mirOperand = &mirObjectList->value;
                if (mirOperand->type == OPERAND_IDENTITY) {
                    int valueRegIndex = loadVarIntoReg(mirCode, mirOperand);
                    int greaterRegisterWidth = getOperandSize(mirOperand);
                    const char *valueReg = getCommonRegName(
                            valueRegIndex,
                            greaterRegisterWidth
                    );
                    int paramRegIndex = allocParamReg(paramIndex);
                    commonRegsVarName[paramRegIndex] = mirOperand->identity;
                    const char *paramRegName = getCommonRegName(
                            paramRegIndex,
                            greaterRegisterWidth
                    );
                    binaryOp2(mov,
                              greaterRegisterWidth == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[paramRegIndex],
                              commonRegisterBinary[valueRegIndex],
                              false);
                    writeFile("\tmov\t%s, %s\n", paramRegName, valueReg);
                    free(((void *) paramRegName));
                } else if (mirOperand->type == OPERAND_RET) {
                    int paramRegIndex = allocParamReg(paramIndex);
                    commonRegsVarName[paramRegIndex] = "ret";
                    const char *paramRegName = getCommonRegName(
                            paramRegIndex,
                            ARM_BLOCK_64_ALIGN
                    );
                    binaryOp2(mov,
                              ARM_BLOCK_64_ALIGN,
                              commonRegisterBinary[paramRegIndex],
                              X0,
                              false);
                    writeFile("\tmov\t%s, %s\n", paramRegName, getCommonRegName(
                            0,
                            ARM_BLOCK_64_ALIGN
                    ));
                    free(((void *) paramRegName));
                } else {
                    //value1 must be reg!!!
                    int paramRegIndex = allocParamReg(paramIndex);
                    char *paramVarName = (char *) malloc(sizeof(char) * 3);
                    snprintf(paramVarName, 3, "%d", paramIndex);
                    commonRegsVarName[paramRegIndex] = paramVarName;
                    const char *paramRegName = getCommonRegName(
                            paramRegIndex,
                            getPrimitiveMirOperandSizeInByte(mirOperand->type)
                    );
                    binaryOp2(mov,
                              getPrimitiveMirOperandSizeInByte(mirOperand->type) == ARM_BLOCK_32_ALIGN,
                              commonRegisterBinary[paramRegIndex],
                              convertMirOperandImmBinary(mirOperand),
                              true);
                    writeFile("\tmov\t%s, %s\n", paramRegName, convertMirOperandAsm(mirOperand));
                    free(((void *) paramRegName));
                }
                mirObjectList = mirObjectList->next;
                paramIndex++;
            }
            //jump to method label
            binaryOpBranch(bl, UNUSED, mirCall->label);
            writeFile("\tbl\t%s\n", mirCall->label);
            clearRegs();
            break;
        }
        case MIR_LABEL: {
            emitLabel(mirCode->mirLabel->label);
            writeFile("%s:\n", mirCode->mirLabel->label);
            break;
        }
        case MIR_RET: {
            MirRet *mirRet = mirCode->mirRet;
            MirOperand *mirOperand = mirRet->value;
            if (mirOperand == nullptr) {
                //return void
                binaryOp2(mov, false, X0, XZR, false);
                writeFile("\tmov\tx0, xzr\n");
            } else {
                //return value
                if (mirOperand->type == OPERAND_IDENTITY) {
                    int valueRegIndex = loadVarIntoReg(mirCode, mirOperand);
                    int valueRegisterWidth = getOperandSize(mirOperand);
                    binaryOp2(mov, false, X0, valueRegIndex, false);
                    const char *value = getCommonRegName(
                            valueRegIndex,
                            getVarSizeFromStack(mirOperand->identity)
                    );

                    writeFile("\tmov\t%s, %s\n", getCommonRegName(
                            0,
                            getVarSizeFromStack(mirOperand->identity)
                    ), value);
                    free(((void *) value));
                } else if (mirOperand->type == OPERAND_RET) {
                    //do nothing
                    //because we do not need "mov X0, X0"
                } else {
                    //value1 must be reg!!!
                    const char *value = convertMirOperandAsm(mirOperand);
                    binaryOp2(mov, false, X0, convertMirOperandImmBinary(mirOperand), true);
                    writeFile("\tmov\t%s, %s\n", getCommonRegName(
                            0,
                            getPrimitiveMirOperandSizeInByte(mirOperand->type)
                    ), value);
                    free(((void *) value));
                }
            }
            commonRegsVarName[0] = "ret";
            //move the value to x0
            break;
        }
        case MIR_JMP: {
            binaryOpBranch(b, UNUSED, mirCode->mirLabel->label);
            writeFile("\tb\t%s\n", mirCode->mirLabel->label);
            break;
        }
        case MIR_CMP: {
            MirCmp *mirCmp = mirCode->mirCmp;
            //value1
            MirOperand *mirOperand1 = &mirCmp->value1;
            MirOperand *mirOperand2 = &mirCmp->value2;
            int value1Size = getOperandSize(mirOperand1);
            int value2Size = getOperandSize(mirOperand2);
            int greaterSize = value1Size > value2Size ? value1Size : value2Size;

            const char *value1 = nullptr;
            int value1RegIndex = -1;
            if (mirOperand1->type == OPERAND_IDENTITY) {
                value1RegIndex = loadVarIntoReg(mirCode, mirOperand1);
                value1 = getCommonRegName(
                        value1RegIndex,
                        greaterSize
                );
            } else if (mirOperand1->type == OPERAND_RET) {
                value1RegIndex = 0;
                value1 = getCommonRegName(
                        0,
                        ARM_BLOCK_64_ALIGN
                );
            } else {
                //value1 must be reg!!!
                value1RegIndex = allocEmptyReg(mirCode);
                value1 = getCommonRegName(
                        value1RegIndex,
                        greaterSize
                );
                binaryOp2(mov,
                          greaterSize == ARM_BLOCK_32_ALIGN,
                          commonRegisterBinary[value1RegIndex],
                          convertMirOperandImmBinary(mirOperand1),
                          true);
                writeFile("\tmov\t%s, %s\n", value1, convertMirOperandAsm(mirOperand1));
            }
            commonRegsVarName[value1RegIndex] = "cmp_value1";
            //value2

            const char *value2 = nullptr;
            int value2RegIndex = -1;
            int value2Imm = 0;
            if (mirOperand2->type == OPERAND_IDENTITY) {
                value2RegIndex = loadVarIntoReg(mirCode, mirOperand2);
                value2 = getCommonRegName(
                        value2RegIndex,
                        greaterSize
                );
                commonRegsVarName[value2RegIndex] = "cmp_value2";
            } else if (mirOperand2->type == OPERAND_RET) {
                value2RegIndex = 0;
                value2 = getCommonRegName(
                        0,
                        ARM_BLOCK_64_ALIGN
                );
                commonRegsVarName[0] = "cmp_value2";
            } else {
                //value2 can be a imm
                value2 = convertMirOperandAsm(mirOperand2);
                value2RegIndex = -1;
                value2Imm = convertMirOperandImmBinary(mirOperand2);
            }
            binaryOp2(
                    cmp,
                    greaterSize == ARM_BLOCK_64_ALIGN,
                    commonRegisterBinary[value1RegIndex],
                    value2RegIndex != -1 ? commonRegisterBinary[value2RegIndex] : value2Imm,
                    value2RegIndex == -1
            );
            writeFile("\tcmp\t%s, %s\n", value1, value2);
            free(((void *) value1));
            free(((void *) value2));
            //op
            switch (mirCmp->op) {
                case CMP_G: {
                    binaryOpBranch(bc, GT, mirCmp->trueLabel->label);
                    writeFile("\tb.gt\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_L: {
                    binaryOpBranch(bc, LT, mirCmp->trueLabel->label);
                    writeFile("\tb.lt\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_GE: {
                    binaryOpBranch(bc, GE, mirCmp->trueLabel->label);
                    writeFile("\tb.ge\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_LE: {
                    binaryOpBranch(bc, LE, mirCmp->trueLabel->label);
                    writeFile("\tb.le\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_E: {
                    binaryOpBranch(bc, EQ, mirCmp->trueLabel->label);
                    writeFile("\tb.eq\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_NE: {
                    binaryOpBranch(bc, NE, mirCmp->trueLabel->label);
                    writeFile("\tb.ne\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_UNKNOWN:
                default: {
                    loge(ARM64_TAG, "unknown cmp op:%d", mirCmp->op);
                    break;
                }
            }
            if (mirCmp->falseLabel != nullptr) {
                binaryOpBranch(b, UNUSED, mirCmp->falseLabel->label);
                writeFile("\tb\t%s\n", mirCmp->falseLabel->label);
            }
            break;
        }
        case MIR_OPT_FLAG: {
            if (mirCode->optFlag == OPT_ENTER_LOOP_BLOCK) {
                clearRegs();
                writeFile(";loop start\n");
            } else if (mirCode->optFlag == OPT_EXIT_LOOP_BLOCK) {
                clearRegs();
                writeFile(";loop end\n");
            }
            break;
        }
        default: {
            loge(ARM64_TAG, "unknown MIR: %d", mirCode->mirType);
        }
    }
}

/**
 * complete!
 * @param mirMethod
 * @return
 */
int computeMethodStackSize(MirMethod *mirMethod) {
    int stackSizeInByte = 0;
    //compute param size
    MirMethodParam *mirMethodParam = mirMethod->param;
    while (mirMethodParam != nullptr) {
        stackSizeInByte += alignBlockSize(mirMethodParam->byte);
        mirMethodParam = mirMethodParam->next;
    }
    //compute var size
    vector<char *> varList;
    MirCode *mirCode = mirMethod->code;
    while (mirCode != nullptr) {
        switch (mirCode->mirType) {
            case MIR_3: {
                bool varFound = false;
                for (int i = 0; i < varList.size(); i++) {
                    if (strcmp(varList[i], mirCode->mir3->distIdentity) == 0) {
                        varFound = true;
                        break;
                    }
                }
                if (!varFound) {
                    int size = getPrimitiveMirOperandSizeInByte(mirCode->mir3->distType);
//                    logd(ARM64_TAG, "[+] new var %s size %d", mirCode->mir3->distIdentity, size);
                    stackSizeInByte += alignBlockSize(size);
                    varList.push_back(mirCode->mir3->distIdentity);
                }
                break;
            }
            case MIR_2: {
                bool varFound = false;
                for (int i = 0; i < varList.size(); i++) {
                    if (strcmp(varList[i], mirCode->mir2->distIdentity) == 0) {
                        varFound = true;
                        break;
                    }
                }
                if (!varFound) {
                    int size = getPrimitiveMirOperandSizeInByte(mirCode->mir2->distType);
                    stackSizeInByte += alignBlockSize(size);
//                    logd(ARM64_TAG, "[+] new var %s size %d", mirCode->mir2->distIdentity, size);
                    varList.push_back(mirCode->mir2->distIdentity);
                }
                break;
            }
            default: {
                break;
            }
        }
        mirCode = mirCode->nextCode;
    }

    return stackSizeInByte;
}

void generateText(MirMethod *mirMethod) {
    writeFile("\t.globl %s\n", mirMethod->label);
    writeFile("\t.p2align %d\n", PAGE_ALIGNMENT);
    emitLabel(mirMethod->label);
    writeFile("%s:\n", mirMethod->label);
    clearRegs();
    int methodStackSize = computeMethodStackSize(mirMethod);
    allocStack(methodStackSize);
    storeParamsToStack(mirMethod->param);
    MirCode *code = mirMethod->code;
    while (code != nullptr) {
        generateCodes(code);
        code = code->nextCode;
    }
    releaseStack(methodStackSize);
    binaryOpRet(ret);
    writeFile("\tret\n\n");
}

void generateData(MirData *mirData) {
    //todo
}

void generateArm64Target(Mir *mir,
                         int _outputAssembly,
                         Platform platform,
                         const char *outputFileName) {
    logd(ARM64_TAG, "arm64 target generation...");
    openFile(outputFileName);
    outputAssembly = _outputAssembly;
    //.text
    generateSection(SECTION_TEXT);
    MirMethod *mirMethod = mir->mirMethod;
    while (mirMethod != nullptr) {
        generateText(mirMethod);
        mirMethod = mirMethod->next;
    }

    //.data
    if (mir->mirData != nullptr) {
        generateSection(SECTION_DATA);
        MirData *mirData = mir->mirData;
        while (mirData != nullptr) {
            generateData(mirData);
            mirData = mirData->next;
        }
    }
    closeFile();
}