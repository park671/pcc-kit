//
// Created by Park Yu on 2024/9/27.
//

#include "optimization.h"
#include "logger.h"
#include <string.h>

#define OPT_TAG "optimization"

bool replaceFutureUsedMir2(MirCode *currentMirCode, const char *identity, MirOperand *replaceWith) {
    bool result = false;
    MirCode *mirCode = currentMirCode->nextCode;
    bool currentInLoop = false;
    while (mirCode != nullptr) {
        //deal with block
        if (mirCode->mirType == MIR_OPT_FLAG) {
            //for while
            if (mirCode->optFlag == OPT_ENTER_LOOP_BLOCK) {
                currentInLoop = true;
            } else if (mirCode->optFlag == OPT_EXIT_LOOP_BLOCK) {
                currentInLoop = false;
            }
        }
        if (currentInLoop) {
            if (mirCode->mirType == MIR_2) {
                Mir2 *mir2 = mirCode->mir2;
                if (strcmp(mir2->distIdentity, identity) == 0 && currentInLoop) {
                    return false;
                }
            } else if (mirCode->mirType == MIR_3) {
                Mir3 *mir3 = mirCode->mir3;
                if (strcmp(mir3->distIdentity, identity) == 0 && currentInLoop) {
                    return false;
                }
            }
        }
        mirCode = mirCode->nextCode;
    }

    mirCode = currentMirCode->nextCode;
    int blockLevel = 0;
    while (mirCode != nullptr) {
        //deal with block
        if (mirCode->mirType == MIR_OPT_FLAG) {
            //if
            if (mirCode->optFlag == OPT_ENTER_BLOCK
                || mirCode->optFlag == OPT_ENTER_LOOP_BLOCK) {
                blockLevel++;
            } else if (mirCode->optFlag == OPT_EXIT_BLOCK
                       || mirCode->optFlag == OPT_EXIT_LOOP_BLOCK) {
                blockLevel--;
                if (blockLevel < 0) {
                    break;
                }
            }
        }

        if (mirCode->mirType == MIR_2) {
            Mir2 *mir2 = mirCode->mir2;
            if (mir2->fromValue.type.primitiveType == OPERAND_IDENTITY &&
                strcmp(mir2->fromValue.identity, identity) == 0) {
                mir2->fromValue = *replaceWith;
                result = true;
            }
            if (strcmp(mir2->distIdentity, identity) == 0) {
                break;
            }
        } else if (mirCode->mirType == MIR_3) {
            Mir3 *mir3 = mirCode->mir3;
            if (mir3->value1.type.primitiveType == OPERAND_IDENTITY && strcmp(mir3->value1.identity, identity) == 0) {
                mir3->value1 = *replaceWith;
                result = true;
            }
            if (mir3->value2.type.primitiveType == OPERAND_IDENTITY && strcmp(mir3->value2.identity, identity) == 0) {
                mir3->value2 = *replaceWith;
                result = true;
            }
            if (strcmp(mir3->distIdentity, identity) == 0) {
                break;
            }
        } else if (mirCode->mirType == MIR_RET) {
            MirRet *mirRet = mirCode->mirRet;
            if (mirRet->value != nullptr) {
                if (mirRet->value->type.primitiveType == OPERAND_IDENTITY &&
                    strcmp(mirRet->value->identity, identity) == 0) {
                    mirRet->value = replaceWith;
                    result = true;
                }
            }
        } else if (mirCode->mirType == MIR_CMP) {
            MirCmp *mirCmp = mirCode->mirCmp;
            if (mirCmp->value1.type.primitiveType == OPERAND_IDENTITY &&
                strcmp(mirCmp->value1.identity, identity) == 0) {
                mirCmp->value1 = *replaceWith;
                result = true;
            }
            if (mirCmp->value2.type.primitiveType == OPERAND_IDENTITY &&
                strcmp(mirCmp->value2.identity, identity) == 0) {
                mirCmp->value2 = *replaceWith;
                result = true;
            }
        } else if (mirCode->mirType == MIR_CALL) {
            MirCall *mirCall = mirCode->mirCall;
            MirObjectList *mirObjectList = mirCall->mirObjectList;
            while (mirObjectList != nullptr) {
                if (mirObjectList->value.type.primitiveType == OPERAND_IDENTITY &&
                    strcmp(mirObjectList->value.identity, identity) == 0) {
                    mirObjectList->value = *replaceWith;
                    result = true;
                }
                mirObjectList = mirObjectList->next;
            }
        }
        mirCode = mirCode->nextCode;
    }
    return result;
}

/**
 *
 * @param mirMethod
 * @param currentMirCode
 * @return head
 */
MirCode *removeMirCode(MirMethod *mirMethod, MirCode *currentMirCode) {
    MirCode *mirCode = mirMethod->code;
    MirCode *preMirCode = nullptr;
    while (mirCode != nullptr && mirCode != currentMirCode) {
        preMirCode = mirCode;
        mirCode = mirCode->nextCode;
    }
    if (mirCode != nullptr) {
        //found & remove
        if (preMirCode != nullptr) {
            preMirCode->nextCode = mirCode->nextCode;
            return mirMethod->code;
        } else {
            //remove head?
            return mirMethod->code->nextCode;
        }
    }
    return mirMethod->code;
}

void printMirCode(MirMethod *mirMethod);

void foldMir2(MirMethod *mirMethod) {
    MirCode *mirCode = mirMethod->code;
    while (mirCode != nullptr) {
        if (mirCode->mirType == MIR_2
            && mirCode->mir2->op == OP_ASSIGNMENT
            && !mirCode->mir2->fromValue.type.isPointer) {
            //todo this is a BIG shit, will fix in future
            Mir2 *mir2 = mirCode->mir2;
            if (replaceFutureUsedMir2(mirCode, mir2->distIdentity, &mir2->fromValue)) {
                mirMethod->code = removeMirCode(mirMethod, mirCode);
            }
        }
        mirCode = mirCode->nextCode;
    }
}

Mir *optimize(Mir *mir, int optimizationLevel) {
    if (optimizationLevel > 0) {
        logd(OPT_TAG, "optimizing...");
        MirMethod *mirMethod = mir->mirMethod;
        while (mirMethod != nullptr) {
            foldMir2(mirMethod);
            mirMethod = mirMethod->next;
        }
    }
//    printMirCode(mir);
    return mir;
}