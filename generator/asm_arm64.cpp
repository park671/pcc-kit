//
// Created by Park Yu on 2024/9/23.
//

#include <stdio.h>
#include <vector>
#include "asm_arm64.h"
#include "logger.h"
#include "file.h"
#include "register_arm64.h"

using namespace std;

#define ASM_TAG "arm64_asm"

#define ARM_STACK_ALIGN 16
#define ARM_BLOCK_32_ALIGN 4
#define ARM_BLOCK_64_ALIGN 8

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
    writeFile("\tsub\tsp, sp, #%d\n", currentStackTop);
    currentStackTop -= 16;
    writeFile("\tstp\tx29, x30, [sp, #%d]\n", currentStackTop);//2 x 64bit = 16byte
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
    loge(ASM_TAG, "unknown var size %s", varName);
    return -1;
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
    writeFile("\tldp\tx29, x30, [sp, #%d]\n", currentStackTop - 16);
    writeFile("\tadd\tsp, sp, #%d\n", currentStackTop);
}

//store reg to stack
void emitStr(const char *regName, int stackOffset) {
    writeFile("\tstr\t%s, [sp, #%d]\n", regName, stackOffset);
}

void storeParamsToStack(MirMethodParam *param) {
    //from x0 - x7
    int paramIndex = 0;
    while (param != nullptr) {
        int offset = allocVarFromStack(param->paramName, param->byte);
        const char *regName = getCommonRegName(paramIndex, param->byte);
        emitStr(regName, offset);
        free((void *) regName);
        param = param->next;
        paramIndex++;
    }
}

/**
 * complete
 * @param mirOperandType
 * @return
 */
int getMirOperandSizeInByte(MirOperandType mirOperandType) {
    switch (mirOperandType) {
        case OPERAND_INT8:
            return 1;
        case OPERAND_INT16:
            return 2;
        case OPERAND_INT32:
            return 4;
        case OPERAND_INT64:
            return 8;
        case OPERAND_FLOAT32:
            return 4;
        case OPERAND_FLOAT64:
            return 8;
        case OPERAND_UNKNOWN:
        case OPERAND_IDENTITY:
        case OPERAND_RET:
        case OPERAND_VOID:
        default: {
            loge(ASM_TAG, "internal error: invalid type for sizing %d", mirOperandType);
            return 0;
        }
    }
}

/**
 * complete
 * @param mirOperand
 * @return index of common reg
 */
int loadVarIntoReg(MirCode *mirCode, MirOperand *mirOperand) {
    if (mirOperand->type != OPERAND_IDENTITY) {
        loge(ASM_TAG, "internal error: get var but not identity type operand");
        return -1;
    }
    int fromRegIndex = isVarExistInCommonReg(mirOperand->identity);
    if (fromRegIndex == -1) {
        int stackOffset = getVarFromStack(mirOperand->identity);
        if (stackOffset == -1) {
            loge(ASM_TAG, "internal error: can not found var %s on stack", mirOperand->identity);
            return -1;
        }
        fromRegIndex = allocEmptyReg(mirCode);
        const char *fromRegName = getCommonRegName(
                fromRegIndex,
                getVarSizeFromStack(mirOperand->identity)
        );
        writeFile("\tldr\t%s, [sp, #%d]\n", fromRegName, stackOffset);
        free(((void *) fromRegName));
        commonRegsVarName[fromRegIndex] = mirOperand->identity;
    }
    return fromRegIndex;
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
            loge(ASM_TAG, "invalid operand for convert %d", mirOperand->type);
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
            int distRegWidth = getMirOperandSizeInByte(mir2->distType);
            const char *distRegName = getCommonRegName(
                    distRegIndex,
                    distRegWidth
            );

            MirOperand *mirOperand = &mir2->fromValue;
            if (mirOperand->type == OPERAND_IDENTITY) {
                int fromRegIndex = isVarExistInCommonReg(mirOperand->identity);
                if (fromRegIndex == -1) {
                    int stackOffset = getVarFromStack(mirOperand->identity);
                    if (stackOffset == -1) {
                        loge(ASM_TAG, "internal error: can not found var %s on stack", mirOperand->identity);
                    }
                    writeFile("\tldr\t%s, [sp, #%d]\n", distRegName, stackOffset);
                } else {
                    const char *fromRegName = getCommonRegName(
                            fromRegIndex,
                            getVarSizeFromStack(mirOperand->identity)
                    );
                    writeFile("\tmov\t%s, %s\n", distRegName, fromRegName);
                    free(((void *) fromRegName));
                }
            } else {
                writeFile("\tmov\t%s, ", distRegName);
                switch (mirOperand->type) {
                    case OPERAND_RET: {
                        //ret must be x0
                        writeFile("%s", getCommonRegName(
                                0,
                                distRegWidth
                        ));
                        break;
                    }
                    case OPERAND_INT8:
                    case OPERAND_INT16:
                    case OPERAND_INT32: {
                        writeFile("#%d", mirOperand->dataInt32);
                        break;
                    }
                    case OPERAND_INT64: {
                        writeFile("#%lld", mirOperand->dataInt64);
                        break;
                    }
                    case OPERAND_FLOAT32:
                    case OPERAND_FLOAT64: {
                        writeFile("#%f", mirOperand->dataFloat64);
                        break;
                    }
                }
                writeFile("\n");
            }
            commonRegsVarName[distRegIndex] = mir2->distIdentity;
            int distStackOffset = getVarFromStack(mir2->distIdentity);
            if (distStackOffset == -1) {
                int distSize = getMirOperandSizeInByte(mir2->distType);
                distStackOffset = allocVarFromStack(mir2->distIdentity, distSize);
            }
            writeFile("\tstr\t%s, [sp, #%d]\n", distRegName, distStackOffset);
            free(((void *) distRegName));
            break;
        }
        case MIR_3: {
            Mir3 *mir3 = mirCode->mir3;
            //value1
            MirOperand *mirOperand = &mir3->value1;
            const char *value1 = nullptr;
            int value1RegIndex = -1;
            int value1RegWidth = -1;
            if (mirOperand->type == OPERAND_IDENTITY) {
                value1RegIndex = loadVarIntoReg(mirCode, mirOperand);
                value1RegWidth = getVarSizeFromStack(mirOperand->identity);
            } else if (mirOperand->type == OPERAND_RET) {
                value1RegIndex = 0;
                value1RegWidth = ARM_BLOCK_64_ALIGN;
            } else {
                //value1 must be reg!!!
                value1RegIndex = allocEmptyReg(mirCode);
                value1RegWidth = getMirOperandSizeInByte(mirOperand->type);
                value1 = getCommonRegName(
                        value1RegIndex,
                        value1RegWidth
                );
                writeFile("\tmov\t%s, %s\n", value1, convertMirOperandAsm(mirOperand));
            }
            if (value1 == nullptr) {
                value1 = getCommonRegName(
                        value1RegIndex,
                        value1RegWidth
                );
            }
            commonRegsVarName[value1RegIndex] = "mir3_value1";

            //value2
            mirOperand = &mir3->value2;
            const char *value2 = nullptr;
            if (mirOperand->type == OPERAND_IDENTITY) {
                int value2RegIndex = loadVarIntoReg(mirCode, mirOperand);
                value2 = getCommonRegName(
                        value2RegIndex,
                        getVarSizeFromStack(mirOperand->identity)
                );
                commonRegsVarName[value2RegIndex] = "mir3_value2";
            } else if (mirOperand->type == OPERAND_RET) {
                value2 = getCommonRegName(
                        0,
                        ARM_BLOCK_64_ALIGN
                );
                commonRegsVarName[0] = "mir3_value2";
            } else {
                if (mir3->op == OP_ADD || mir3->op == OP_SUB) {
                    //value2 can be a imm
                    value2 = convertMirOperandAsm(mirOperand);
                } else {
                    //stupid arm64 do not support imm in mul & div inst
                    //value2 must be reg!!!
                    int value2RegIndex = allocEmptyReg(mirCode);
                    value2 = getCommonRegName(
                            value2RegIndex,
                            getMirOperandSizeInByte(mirOperand->type)
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
                        value1RegWidth
                );;
            } else {
                dist = getCommonRegName(
                        distRegIndex,
                        getMirOperandSizeInByte(mir3->distType)
                );
            }

            //op
            switch (mir3->op) {
                case OP_UNKNOWN:
                    break;
                case OP_ADD:
                    writeFile("\tadd\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_SUB:
                    writeFile("\tsub\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_MUL:
                    writeFile("\tmul\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_DIV:
                    writeFile("\tdiv\t%s, %s, %s\n", dist, value1, value2);
                    break;
                case OP_MOD:
                    loge(ASM_TAG, "[-] not impl yet!");
                    break;
                case OP_ASSIGNMENT:
                    break;
            }
            commonRegsVarName[distRegIndex] = mir3->distIdentity;
            int distStackOffset = getVarFromStack(mir3->distIdentity);
            if (distStackOffset == -1) {
                int distSize = getMirOperandSizeInByte(mir3->distType);
                distStackOffset = allocVarFromStack(mir3->distIdentity, distSize);
            }
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
                    const char *valueReg = getCommonRegName(
                            valueRegIndex,
                            getVarSizeFromStack(mirOperand->identity)
                    );
                    int paramRegIndex = allocParamReg(paramIndex);
                    commonRegsVarName[paramRegIndex] = mirOperand->identity;
                    const char *paramRegName = getCommonRegName(
                            paramRegIndex,
                            getVarSizeFromStack(mirOperand->identity)
                    );
                    writeFile("\tmov\t%s, %s\n", paramRegName, valueReg);
                    free(((void *) paramRegName));
                } else if (mirOperand->type == OPERAND_RET) {
                    int paramRegIndex = allocParamReg(paramIndex);
                    commonRegsVarName[paramRegIndex] = "ret";
                    const char *paramRegName = getCommonRegName(
                            paramRegIndex,
                            ARM_BLOCK_64_ALIGN
                    );
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
                            getMirOperandSizeInByte(mirOperand->type)
                    );
                    writeFile("\tmov\t%s, %s\n", paramRegName, convertMirOperandAsm(mirOperand));
                    free(((void *) paramRegName));
                }
                mirObjectList = mirObjectList->next;
                paramIndex++;
            }
            //jump to method label
            writeFile("\tbl\t%s\n", mirCall->label);
            clearRegs();
            break;
        }
        case MIR_LABEL: {
            writeFile("%s:\n", mirCode->mirLabel->label);
            break;
        }
        case MIR_RET: {
            MirRet *mirRet = mirCode->mirRet;
            MirOperand *mirOperand = mirRet->value;
            if (mirOperand == nullptr) {
                //return void
                writeFile("\tmov\tx0, xzr\n");
            } else {
                //return value
                if (mirOperand->type == OPERAND_IDENTITY) {
                    int valueRegIndex = loadVarIntoReg(mirCode, mirOperand);
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
                } else {
                    //value1 must be reg!!!
                    const char *value = convertMirOperandAsm(mirOperand);
                    writeFile("\tmov\t%s, %s\n", getCommonRegName(
                            0,
                            getMirOperandSizeInByte(mirOperand->type)
                    ), value);
                    free(((void *) value));
                }
            }
            commonRegsVarName[0] = "ret";
            //move the value to x0
            break;
        }
        case MIR_JMP: {
            writeFile("\tb\t%s\n", mirCode->mirLabel->label);
            break;
        }
        case MIR_CMP: {
            MirCmp *mirCmp = mirCode->mirCmp;
            //value1
            MirOperand *mirOperand = &mirCmp->value1;
            const char *value1 = nullptr;
            int value1RegIndex = -1;
            if (mirOperand->type == OPERAND_IDENTITY) {
                value1RegIndex = loadVarIntoReg(mirCode, mirOperand);
                value1 = getCommonRegName(
                        value1RegIndex,
                        getVarSizeFromStack(mirOperand->identity)
                );
            } else if (mirOperand->type == OPERAND_RET) {
                value1 = getCommonRegName(
                        0,
                        ARM_BLOCK_64_ALIGN
                );
            } else {
                //value1 must be reg!!!
                value1RegIndex = allocEmptyReg(mirCode);
                value1 = getCommonRegName(
                        value1RegIndex,
                        getMirOperandSizeInByte(mirOperand->type)
                );
                writeFile("\tmov\t%s, %s\n", value1, convertMirOperandAsm(mirOperand));
            }
            commonRegsVarName[value1RegIndex] = "cmp_value1";
            //value2
            mirOperand = &mirCmp->value2;
            const char *value2 = nullptr;
            if (mirOperand->type == OPERAND_IDENTITY) {
                int value2RegIndex = loadVarIntoReg(mirCode, mirOperand);
                value2 = getCommonRegName(
                        value2RegIndex,
                        getVarSizeFromStack(mirOperand->identity)
                );
                commonRegsVarName[value2RegIndex] = "cmp_value2";
            } else if (mirOperand->type == OPERAND_RET) {
                value2 = getCommonRegName(
                        0,
                        ARM_BLOCK_64_ALIGN
                );
                commonRegsVarName[0] = "cmp_value2";
            } else {
                //value2 can be a imm
                value2 = convertMirOperandAsm(mirOperand);
            }
            writeFile("\tcmp\t%s, %s\n", value1, value2);
            free(((void *) value1));
            free(((void *) value2));
            //op
            switch (mirCmp->op) {
                case CMP_G: {
                    writeFile("\tb.gt\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_L: {
                    writeFile("\tb.lt\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_GE: {
                    writeFile("\tb.ge\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_LE: {
                    writeFile("\tb.le\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_E: {
                    writeFile("\tb.eq\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_NE: {
                    writeFile("\tb.ne\t%s\n", mirCmp->trueLabel->label);
                    break;
                }
                case CMP_UNKNOWN:
                default: {
                    loge(ASM_TAG, "unknown cmp op:%d", mirCmp->op);
                    break;
                }
            }
            if (mirCmp->falseLabel != nullptr) {
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
            loge(ASM_TAG, "unknown MIR: %d", mirCode->mirType);
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
                    int size = getMirOperandSizeInByte(mirCode->mir3->distType);
//                    logd(ASM_TAG, "[+] new var %s size %d", mirCode->mir3->distIdentity, size);
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
                    int size = getMirOperandSizeInByte(mirCode->mir2->distType);
                    stackSizeInByte += alignBlockSize(size);
//                    logd(ASM_TAG, "[+] new var %s size %d", mirCode->mir2->distIdentity, size);
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
    writeFile("\tret\n\n");
}

void generateData(MirData *mirData) {
    //todo
}

void generateArm64Asm(Mir *mir, const char *assemblyFileName) {
    logd(ASM_TAG, "assembly generation...");
    openFile(assemblyFileName);
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