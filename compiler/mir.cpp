#include <cstdlib>
#include <cstdio>
#include <string.h>
#include "mir.h"
#include "logger.h"

#define MIR_TAG "mir"

static MirCode *firstMirCode;
static MirCode *lastMirCode;
static bool mirCodeSessionStarted = false;

static const char *lastCalledMethodIdentity = nullptr;

struct VarNode {
    const char *identity;
    MirOperandType operandType;

    VarNode *next;
};

static VarNode *currentStackVarNodeHead = nullptr;

VarNode *getVarInfo(const char *identity) {
    VarNode *pVarNode = currentStackVarNodeHead;
    while (pVarNode != nullptr) {
        if (pVarNode->identity == identity || strcmp(pVarNode->identity, identity) == 0) {
            return pVarNode;
        }
        pVarNode = pVarNode->next;
    }
    return nullptr;
}

void addVarInfo(const char *identity, MirOperandType operandType) {
    if (operandType < 3) {
        loge(MIR_TAG, "internal error: operand type error");
        return;
    }
    VarNode *varNode = (VarNode *) malloc(sizeof(VarNode));
    varNode->identity = identity;
    varNode->operandType = operandType;
    if (currentStackVarNodeHead == nullptr) {
        currentStackVarNodeHead = varNode;
    } else {
        VarNode *pVarNode = currentStackVarNodeHead;
        while (pVarNode->next != nullptr) {
            pVarNode = pVarNode->next;
        }
        pVarNode->next = varNode;
    }
    varNode->next = nullptr;
}

void resetVarInfo() {
    VarNode *pVarNode = currentStackVarNodeHead;
    while (pVarNode != nullptr) {
        VarNode *need2ReleaseNode = pVarNode;
        pVarNode = pVarNode->next;
        free(need2ReleaseNode);
    }
    currentStackVarNodeHead = nullptr;
}


struct MethodNode {
    const char *identity;
    MirOperandType operandType;

    MethodNode *next;
};

static MethodNode *methodNodeHead = nullptr;

MethodNode *getMethodInfo(const char *identity) {
    MethodNode *pMethodNode = methodNodeHead;
    while (pMethodNode != nullptr) {
        if (pMethodNode->identity == identity || strcmp(pMethodNode->identity, identity) == 0) {
            return pMethodNode;
        }
        pMethodNode = pMethodNode->next;
    }
    return nullptr;
}

void addMethodInfo(const char *identity, MirOperandType operandType) {
    if (operandType < 3) {
        loge(MIR_TAG, "internal error: operand type error");
        return;
    }
    MethodNode *pMethodNode = (MethodNode *) malloc(sizeof(MethodNode));
    pMethodNode->identity = identity;
    pMethodNode->operandType = operandType;
    if (methodNodeHead == nullptr) {
        methodNodeHead = pMethodNode;
    } else {
        MethodNode *methodNode = methodNodeHead;
        while (methodNode->next != nullptr) {
            methodNode = methodNode->next;
        }
        methodNode->next = pMethodNode;
    }
    pMethodNode->next = nullptr;
}

static void startMirCodeSession() {
    if (firstMirCode != nullptr || mirCodeSessionStarted) {
        loge(MIR_TAG, "[-] already in mir code session");
        return;
    }
    mirCodeSessionStarted = true;
}

static void finishMirCodeSession() {
    if (!mirCodeSessionStarted) {
        loge(MIR_TAG, "[-] no mir code session exist");
        return;
    }
    firstMirCode = nullptr;
    lastMirCode = nullptr;
    mirCodeSessionStarted = false;
    resetVarInfo();
}

static MirCode *getMirCodeSessionHead() {
    return firstMirCode;
}

static int mirCodeLine = 0;

static void emitMirCode(MirCode *mirCode) {
    mirCode->nextCode = nullptr;
    mirCode->codeLine = mirCodeLine++;
//    printMirCode(mirCode);
    if (firstMirCode == nullptr) {
        firstMirCode = mirCode;
    }
    if (lastMirCode != nullptr) {
        lastMirCode->nextCode = mirCode;
    }
    lastMirCode = mirCode;
}

static const char *convertArithmeticOpString(MirOperator mirOperator) {
    switch (mirOperator) {
        case OP_ADD:
            return "+";
        case OP_SUB:
            return "-";
        case OP_MUL:
            return "*";
        case OP_DIV:
            return "/";
        case OP_MOD:
            return "%";
        case OP_ASSIGNMENT:
            return "=";
        case OP_UNKNOWN:
        default:
            return "unknown";
    }
}

static const char *convertBoolOpString(MirBooleanOperator mirBooleanOperator) {
    switch (mirBooleanOperator) {
        case CMP_E:
            return "==";
        case CMP_NE:
            return "!=";
        case CMP_G:
            return ">";
        case CMP_GE:
            return ">=";
        case CMP_L:
            return "<";
        case CMP_LE:
            return "<=";
        case CMP_UNKNOWN:
        default:
            return "unknown";
    }
}


MirOperandType convertAstType2MirType(AstType *retType) {
    if (retType->isPointer) {
        return OPERAND_POINTER;
    }
    PrimitiveType primitiveType = retType->primitiveType;
    switch (primitiveType) {
        case TYPE_CHAR: {
            return OPERAND_INT8;
        }
        case TYPE_SHORT: {
            return OPERAND_INT16;
        }
        case TYPE_INT: {
            return OPERAND_INT32;
        }
        case TYPE_LONG: {
            return OPERAND_INT64;
        }
        case TYPE_FLOAT: {
            return OPERAND_FLOAT32;
        }
        case TYPE_DOUBLE: {
            return OPERAND_FLOAT64;
        }
        case TYPE_VOID: {
            return OPERAND_VOID;
        }
        case TYPE_UNKNOWN:
        default: {
            loge(MIR_TAG, "unknown type:%d", primitiveType);
            exit(1);
        }
    }
    return OPERAND_UNKNOWN;
}

static const char *convertOperand(MirOperand *mirOperand) {
    char *result = nullptr;
    switch (mirOperand->type) {
        case OPERAND_IDENTITY:
            return mirOperand->identity;
        case OPERAND_RET:
            return "[last ret]";
        case OPERAND_INT8:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "%d", mirOperand->dataInt8);
            return result;
        case OPERAND_INT16:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "%d", mirOperand->dataInt16);
            return result;
        case OPERAND_INT32:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "%d", mirOperand->dataInt32);
            return result;
        case OPERAND_INT64:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "%lld", mirOperand->dataInt64);
            return result;
        case OPERAND_FLOAT32:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "%f", mirOperand->dataFloat32);
            return result;
        case OPERAND_FLOAT64:
            result = (char *) malloc(sizeof(char) * 21);
            snprintf(result, 21, "%f", mirOperand->dataFloat64);
            return result;
    }
}

void printMirCode(MirCode *mirCode) {
    switch (mirCode->mirType) {
        case MIR_2: {
            if (mirCode->mir2->distType < 3) {
                loge(MIR_TAG, "error!");
            }
            logd(MIR_TAG, "type %d: %s %s %s",
                 mirCode->mir2->distType,
                 mirCode->mir2->distIdentity,
                 convertArithmeticOpString(mirCode->mir2->op),
                 convertOperand(&mirCode->mir2->fromValue)
            );
            break;
        }
        case MIR_3: {
            if (mirCode->mir2->distType < 3) {
                loge(MIR_TAG, "error!");
            }
            logd(MIR_TAG, "type %d: %s = %s %s %s",
                 mirCode->mir3->distType,
                 mirCode->mir3->distIdentity,
                 convertOperand(&mirCode->mir3->value1),
                 convertArithmeticOpString(mirCode->mir3->op),
                 convertOperand(&mirCode->mir3->value2)
            );
            break;
        }
        case MIR_CMP: {
            MirCmp *mirCmp = mirCode->mirCmp;
            const char *trueLabel = mirCmp->trueLabel->label;
            const char *falseLabel = "nop";
            if (mirCmp->falseLabel->label != nullptr) {
                falseLabel = mirCmp->falseLabel->label;
            }
            logd(MIR_TAG, "cmp: %s %s %s ? %s : %s",
                 convertOperand(&mirCmp->value1),
                 convertBoolOpString(mirCmp->op),
                 convertOperand(&mirCmp->value2),
                 trueLabel,
                 falseLabel
            );
            break;
        }
        case MIR_RET: {
            logd(MIR_TAG, "ret: %s", convertOperand(mirCode->mirRet->value));
            break;
        }
        case MIR_CALL: {
            logd(MIR_TAG, "call: %s(", mirCode->mirCall->label);
            MirCall *mirCall = mirCode->mirCall;
            MirObjectList *mirObjectList = mirCall->mirObjectList;
            while (mirObjectList != nullptr) {
                logd(MIR_TAG, "\t%s", convertOperand(&mirObjectList->value));
                mirObjectList = mirObjectList->next;
            }
            logd(MIR_TAG, ")", mirCode->mirCall->label);
            break;
        }
        case MIR_LABEL: {
            logd(MIR_TAG, "label:%s", mirCode->mirLabel->label);
            break;
        }
        case MIR_JMP: {
            logd(MIR_TAG, "jmp:%s", mirCode->mirLabel->label);
            break;
        }
        case MIR_OPT_FLAG: {
            logd(MIR_TAG, "\t.opt flag:%d", mirCode->optFlag);
            break;
        }
        default: {
            loge(MIR_TAG, "unknown MIR:");
        }
    }
}

static MirCode *createMirCode(MirType mirType) {
    MirCode *mirCode = (MirCode *) malloc(sizeof(MirCode));
    mirCode->mirType = mirType;
    switch (mirType) {
        case MIR_3: {
            mirCode->mir3 = (Mir3 *) malloc(sizeof(Mir3));
            break;
        }
        case MIR_2: {
            mirCode->mir2 = (Mir2 *) malloc(sizeof(Mir2));
            break;
        }
        case MIR_CMP: {
            mirCode->mirCmp = (MirCmp *) malloc(sizeof(MirCmp));
            break;
        }
        case MIR_CALL: {
            mirCode->mirCall = (MirCall *) malloc(sizeof(MirCall));
            break;
        }
        case MIR_RET: {
            mirCode->mirRet = (MirRet *) malloc(sizeof(MirRet));
            break;
        }
            //reuse this
        case MIR_JMP:
        case MIR_LABEL: {
            mirCode->mirLabel = (MirLabel *) malloc(sizeof(MirLabel));
            break;
        }
        case MIR_OPT_FLAG:
            break;
    }
    return mirCode;
}

int tempValueIndex = 0;
int tempLabelIndex = 0;

char *allocTempValue() {
    char *result = (char *) malloc(sizeof(char) * 14);
    snprintf(result, 14, "_tv_%d", tempValueIndex++);
    return result;
}

char *allocTempLabel() {
    char *result = (char *) malloc(sizeof(char) * 14);
    snprintf(result, 14, "_lb_%d", tempLabelIndex++);
    return result;
}

void resetTempValIndex() {
    tempValueIndex = 0;
}

//used for method stack alloc
int getTempValueIndex() {
    return tempValueIndex;
}

void generateExpressionArithmetic(AstExpressionArithmetic *expression, MirOperand *value);

void generateStatementSeq(AstStatementSeq *astStatementSeq);

void generateStatement(AstStatement *astStatement);

void generateStatementMethodCall(
        AstStatementMethodCall *methodCallStatement
);

void generateStatementExpressions(AstStatementExpressions *astStatementExpressions);

void generateExpression(AstExpression *expression, MirOperand *operand);

MirOperator getArithmeticOp(ArithmeticOperatorType operatorType) {
    switch (operatorType) {
        case ARITHMETIC_ADD:
            return OP_ADD;
        case ARITHMETIC_SUB:
            return OP_SUB;
        case ARITHMETIC_MUL:
            return OP_MUL;
        case ARITHMETIC_DIV:
            return OP_DIV;
        case ARITHMETIC_MOD:
            return OP_MOD;
        case ARITHMETIC_NULL:
        default:
            return OP_UNKNOWN;
    }
}

MirBooleanOperator getBoolOp(RelationOperator operatorType) {
    switch (operatorType) {
        case RELATION_EQ:
            return CMP_E;
        case RELATION_NOT_EQ:
            return CMP_NE;
        case RELATION_GREATER:
            return CMP_G;
        case RELATION_GREATER_EQ:
            return CMP_GE;
        case RELATION_LESS:
            return CMP_L;
        case RELATION_LESS_EQ:
            return CMP_LE;
        default:
            return CMP_UNKNOWN;
    }
}

void generateArithmeticFactor(AstArithmeticFactor *arithmeticFactor, MirOperand *mirOperand) {
    switch (arithmeticFactor->factorType) {
        case ARITHMETIC_IDENTITY: {
            mirOperand->type = OPERAND_IDENTITY;
            mirOperand->identity = arithmeticFactor->identity->name;
            break;
        }
        case ARITHMETIC_PRIMITIVE: {
            AstPrimitiveData *primitiveData = arithmeticFactor->primitiveData;
            switch (primitiveData->type) {
                case TYPE_CHAR: {
                    mirOperand->type = OPERAND_INT8;
                    mirOperand->dataInt8 = primitiveData->dataChar;
                    break;
                }
                case TYPE_SHORT: {
                    mirOperand->type = OPERAND_INT16;
                    mirOperand->dataInt16 = primitiveData->dataShort;
                    break;
                }
                case TYPE_INT: {
                    mirOperand->type = OPERAND_INT32;
                    mirOperand->dataInt32 = primitiveData->dataInt;
                    break;
                }
                case TYPE_LONG: {
                    mirOperand->type = OPERAND_INT64;
                    mirOperand->dataInt64 = primitiveData->dataLong;
                    break;
                }
                case TYPE_FLOAT: {
                    mirOperand->type = OPERAND_FLOAT32;
                    mirOperand->dataFloat32 = primitiveData->dataFloat;
                    break;
                }
                case TYPE_DOUBLE: {
                    mirOperand->type = OPERAND_FLOAT64;
                    mirOperand->dataFloat64 = primitiveData->dataDouble;
                    break;
                }
                case TYPE_VOID: {
                    logd(MIR_TAG, "[-] what type void?");
                    mirOperand->type = OPERAND_VOID;
                    break;
                }
                case TYPE_UNKNOWN:
                default: {
                    loge(MIR_TAG, "unknown primitive type.");
                    exit(-1);
                }
            }
            break;
        }
        case ARITHMETIC_METHOD_RET: {
            generateStatementMethodCall(arithmeticFactor->methodCall);
            mirOperand->type = OPERAND_RET;
            mirOperand->retType = convertAstType2MirType(arithmeticFactor->methodCall->retType);
            break;
        }
        default: {
            loge(MIR_TAG, "unknown");
        }
    }
}

void fixMir2Type(Mir2 *mir2) {
    mir2->distType = mir2->fromValue.type;
    if (mir2->distType == OPERAND_IDENTITY) {
        VarNode *varNode = getVarInfo(mir2->fromValue.identity);
        mir2->distType = varNode->operandType;
    } else if (mir2->distType == OPERAND_RET) {
        MethodNode *methodNode = getMethodInfo(lastCalledMethodIdentity);
        lastCalledMethodIdentity = nullptr;
        mir2->distType = methodNode->operandType;
    }
}

void fixMir3Type(Mir3 *mir3) {
    VarNode *varNode = getVarInfo(mir3->distIdentity);
    MirOperandType operandType1 = varNode->operandType;
    MirOperandType operandType2 = mir3->value2.type;
    if (operandType2 == OPERAND_IDENTITY) {
        VarNode *operandType2VarNode = getVarInfo(mir3->value2.identity);
        operandType2 = operandType2VarNode->operandType;
    } else if (operandType2 == OPERAND_RET) {
        MethodNode *methodNode = getMethodInfo(lastCalledMethodIdentity);
        lastCalledMethodIdentity = nullptr;
        operandType2 = methodNode->operandType;
    }
    mir3->distType =
            operandType1 > operandType2 ? operandType1 : operandType2;
}

/**
 * complete!
 * @param arithmeticItem
 * @return
 */
void generateArithmeticItem(AstArithmeticItem *arithmeticItem, MirOperand *value) {
    char *tempVal = allocTempValue();

    MirCode *mirCode = createMirCode(MIR_2);
    mirCode->mir2->distIdentity = tempVal;
    mirCode->mir2->op = OP_ASSIGNMENT;
    generateArithmeticFactor(arithmeticItem->arithmeticFactor, &mirCode->mir2->fromValue);
    fixMir2Type(mirCode->mir2);
    addVarInfo(tempVal, mirCode->mir2->distType);
    emitMirCode(mirCode);

    AstArithmeticItemMore *itemMore = arithmeticItem->arithmeticItemMore;
    while (itemMore != nullptr) {
        mirCode = createMirCode(MIR_3);
        mirCode->mir3->distIdentity = tempVal;
        mirCode->mir3->value1.type = OPERAND_IDENTITY;
        mirCode->mir3->value1.identity = tempVal;
        mirCode->mir3->op = getArithmeticOp(itemMore->arithmeticOperatorType);
        generateArithmeticFactor(itemMore->arithmeticFactor, &mirCode->mir3->value2);
        fixMir3Type(mirCode->mir3);
        emitMirCode(mirCode);
        itemMore = itemMore->arithmeticItemMore;
    }
    value->type = OPERAND_IDENTITY;
    value->identity = tempVal;
}

/**
 *
 * @param expression
 * @param value nullable!!
 */
void generateExpressionAssignment(
        AstExpressionAssignment *expression,
        MirOperand *value
) {
    MirCode *mirCode = createMirCode(MIR_2);
    mirCode->mir2->distIdentity = expression->identity->name;
    mirCode->mir2->op = OP_ASSIGNMENT;
    generateExpression(expression->expression, &mirCode->mir2->fromValue);
    fixMir2Type(mirCode->mir2);
    addVarInfo(mirCode->mir2->distIdentity, mirCode->mir2->distType);
    emitMirCode(mirCode);

    if (value != nullptr) {
        //a = b = xxx, this fromValue is the "b", not "a"
        value->type = OPERAND_IDENTITY;
        //so the "b" is current fromValue name
        value->identity = expression->identity->name;
    }
}

/**
 * complete
 * @param expression
 * @param value
 */
void generateExpressionArithmetic(AstExpressionArithmetic *expression, MirOperand *value) {
    MirCode *mirCode = createMirCode(MIR_2);
    Mir2 *mir2 = mirCode->mir2;
    char *tempVal = allocTempValue();
    mir2->distIdentity = tempVal;
    mir2->op = OP_ASSIGNMENT;
    generateArithmeticItem(expression->arithmeticItem, &mir2->fromValue);
    fixMir2Type(mir2);
    addVarInfo(tempVal, mirCode->mir2->distType);
    emitMirCode(mirCode);
    AstExpressionArithmeticMore *expressionArithmeticMore = expression->arithmeticExpressMore;
    while (expressionArithmeticMore != nullptr) {
        mirCode = createMirCode(MIR_3);
        mirCode->mir3->distIdentity = tempVal;
        mirCode->mir3->value1.type = OPERAND_IDENTITY;
        mirCode->mir3->value1.identity = tempVal;
        mirCode->mir3->op = getArithmeticOp(expressionArithmeticMore->arithmeticOperatorType);
        generateArithmeticItem(expressionArithmeticMore->arithmeticItem, &mirCode->mir3->value2);
        fixMir3Type(mirCode->mir3);
        emitMirCode(mirCode);
        expressionArithmeticMore = expressionArithmeticMore->arithmeticExpressMore;
    }
    if (value != nullptr) {
        //stupid code but it is legal.
        value->type = OPERAND_IDENTITY;
        value->identity = tempVal;
    }
}

/**
 * complete!
 * @param returnStatement
 */
void generateStatementReturn(
        AstStatementReturn *returnStatement
) {
    MirCode *mirCode = createMirCode(MIR_RET);
    if (returnStatement->expression == nullptr) {
        mirCode->mirRet->value = nullptr;
    } else {
        MirOperand *mirOperand = (MirOperand *) malloc(sizeof(MirOperand));
        generateExpression(returnStatement->expression, mirOperand);
        mirCode->mirRet->value = mirOperand;
    }
    emitMirCode(mirCode);
}

/**
 * complete
 * @param objectList
 * @return
 */
MirObjectList *generateObjectList(AstObjectList *objectList) {
    MirObjectList *firstMirObjectList = nullptr;
    MirObjectList *lastMirObjectList = nullptr;
    MirObjectList *mirObjectList = nullptr;
    while (objectList != nullptr) {
        mirObjectList = (MirObjectList *) malloc(sizeof(MirObjectList));
        mirObjectList->next = nullptr;
        if (firstMirObjectList == nullptr) {
            firstMirObjectList = mirObjectList;
        }
        if (lastMirObjectList != nullptr) {
            lastMirObjectList->next = mirObjectList;
        }
        lastMirObjectList = mirObjectList;
        generateExpression(objectList->expression, &mirObjectList->value);
        objectList = objectList->objectMore;
    }
    return firstMirObjectList;
}

/**
 * complete
 * @param methodCallStatement
 */
void generateStatementMethodCall(
        AstStatementMethodCall *methodCallStatement
) {
    MirCode *mirCode = createMirCode(MIR_CALL);
    mirCode->mirCall->label = methodCallStatement->identity->name;
    lastCalledMethodIdentity = mirCode->mirCall->label;
    if (methodCallStatement->objectList == nullptr) {
        mirCode->mirCall->mirObjectList = nullptr;
    } else {
        mirCode->mirCall->mirObjectList = generateObjectList(methodCallStatement->objectList);
    }
    emitMirCode(mirCode);
}

/**
 * complete!
 * @param boolFactor
 * @param trueLabel
 * @param falseLabel
 */
void generateFactorBool(AstBoolFactor *boolFactor, MirLabel *trueLabel, MirLabel *falseLabel) {
    switch (boolFactor->boolFactorType) {
        case BOOL_FACTOR_RELATION: {
            MirCode *mirCode = createMirCode(MIR_CMP);
            MirCmp *mirCmp = mirCode->mirCmp;
            generateExpressionArithmetic(boolFactor->arithmeticBoolFactor->firstArithmeticExpression,
                                         &mirCmp->value1);
            mirCmp->op = getBoolOp(boolFactor->arithmeticBoolFactor->relationOperation);
            generateExpressionArithmetic(boolFactor->arithmeticBoolFactor->secondArithmeticExpression,
                                         &mirCmp->value2);
            mirCmp->trueLabel = trueLabel;
            mirCmp->falseLabel = falseLabel;
            emitMirCode(mirCode);
            break;
        }
        case BOOL_FACTOR_INVERT: {
            //very funny, right? hhh
            generateFactorBool(boolFactor->invertBoolFactor->boolFactor, falseLabel, trueLabel);
            break;
        }
    }

}

/**
 * complete!
 * @param boolItem
 * @param trueLabel
 * @param falseLabel
 */
void generateItemBool(AstBoolItem *boolItem, MirLabel *trueLabel, MirLabel *falseLabel) {
    while (boolItem != nullptr) {
        if (boolItem->next != nullptr) {
            MirCode *mirCode = createMirCode(MIR_LABEL);
            MirLabel *itemTrueLabel = mirCode->mirLabel;
            itemTrueLabel->label = allocTempLabel();
            //any "&&" false, then all false
            generateFactorBool(boolItem->boolFactor, itemTrueLabel, falseLabel);
            emitMirCode(mirCode);
        } else {
            //last "&&", if this true, then all true
            generateFactorBool(boolItem->boolFactor, trueLabel, falseLabel);
        }
        boolItem = boolItem->next;
    }
}

/**
 * complete!
 * @param boolExpression
 * @param trueLabel
 * @param falseLabel
 */
void generateExpressionBool(AstExpressionBool *boolExpression, MirLabel *trueLabel, MirLabel *falseLabel) {
    while (boolExpression != nullptr) {
        if (boolExpression->next != nullptr) {
            MirCode *mirCode = createMirCode(MIR_LABEL);
            MirLabel *expressionFalseLabel = mirCode->mirLabel;
            expressionFalseLabel->label = allocTempLabel();
            //any "||" true, then all true
            generateItemBool(boolExpression->boolItem, trueLabel, expressionFalseLabel);
            emitMirCode(mirCode);
        } else {
            //last "or", if this false, then all false
            generateItemBool(boolExpression->boolItem, trueLabel, falseLabel);
        }
        boolExpression = boolExpression->next;
    }
}

/**
 * complete!
 * @param ifStatement
 */
void generateStatementIf(
        AstStatementIf *ifStatement
) {
    char *trueLabel = allocTempLabel();
    char *falseLabel = allocTempLabel();
    char *finishLabel = allocTempLabel();

    //must have true
    MirCode *trueBranchMirCode = createMirCode(MIR_LABEL);
    trueBranchMirCode->mirLabel->label = trueLabel;
    //must have false, if the statement not declared one, then jump over the "true" blocks.
    MirCode *falseBranchMirCode = createMirCode(MIR_LABEL);
    falseBranchMirCode->mirLabel->label = falseLabel;
    //must have finish, used for "true" blocks jump over false
    MirCode *finishBranchMirCode = createMirCode(MIR_LABEL);
    finishBranchMirCode->mirLabel->label = finishLabel;

    generateExpressionBool(
            ifStatement->expression,
            trueBranchMirCode->mirLabel,
            falseBranchMirCode->mirLabel
    );
    //enter block
    MirCode *optFlag = createMirCode(MIR_OPT_FLAG);
    optFlag->optFlag = OPT_ENTER_BLOCK;
    emitMirCode(optFlag);

    //true
    emitMirCode(trueBranchMirCode);
    generateStatement(ifStatement->trueStatement);

    MirCode *jumpOverFalseMirCode = createMirCode(MIR_JMP);
    jumpOverFalseMirCode->mirLabel->label = finishLabel;
    emitMirCode(jumpOverFalseMirCode);

    //false
    emitMirCode(falseBranchMirCode);
    if (ifStatement->falseStatement != nullptr) {
        generateStatement(ifStatement->falseStatement);
    }
    emitMirCode(finishBranchMirCode);
    //exit block
    optFlag = createMirCode(MIR_OPT_FLAG);
    optFlag->optFlag = OPT_EXIT_BLOCK;
    emitMirCode(optFlag);
}

/**
 * complete!
 * @param whileStatement
 */
void generateStatementWhile(
        AstStatementWhile *whileStatement
) {
    //enter block
    MirCode *optFlag = createMirCode(MIR_OPT_FLAG);
    optFlag->optFlag = OPT_ENTER_LOOP_BLOCK;
    emitMirCode(optFlag);

    //loop entry
    char *loopEntryLabel = allocTempLabel();
    MirCode *loopBranchMirCode = createMirCode(MIR_LABEL);
    loopBranchMirCode->mirLabel->label = loopEntryLabel;
    emitMirCode(loopBranchMirCode);

    char *trueLabel = allocTempLabel();
    char *falseLabel = allocTempLabel();

    //must have true
    MirCode *trueBranchMirCode = createMirCode(MIR_LABEL);
    trueBranchMirCode->mirLabel->label = trueLabel;
    //must have false, if the statement not declared one, then jump over the "true" blocks.
    MirCode *falseBranchMirCode = createMirCode(MIR_LABEL);
    falseBranchMirCode->mirLabel->label = falseLabel;

    generateExpressionBool(
            whileStatement->expression,
            trueBranchMirCode->mirLabel,
            falseBranchMirCode->mirLabel
    );
    //true
    emitMirCode(trueBranchMirCode);
    generateStatement(whileStatement->statement);
    //go back to loop entry
    MirCode *loopBackEntryMirCode = createMirCode(MIR_JMP);
    loopBackEntryMirCode->mirLabel->label = loopEntryLabel;
    emitMirCode(loopBackEntryMirCode);

    //false
    emitMirCode(falseBranchMirCode);
    //exit block
    optFlag = createMirCode(MIR_OPT_FLAG);
    optFlag->optFlag = OPT_EXIT_LOOP_BLOCK;
    emitMirCode(optFlag);
}

void generateStatementFor(
        AstStatementFor *forStatement
) {
    generateExpression(forStatement->initExpression, nullptr);
    //enter block
    MirCode *optFlag = createMirCode(MIR_OPT_FLAG);
    optFlag->optFlag = OPT_ENTER_LOOP_BLOCK;
    emitMirCode(optFlag);

    char *trueLabel = allocTempLabel();
    char *falseLabel = allocTempLabel();
    //must have true
    MirCode *trueBranchMirCode = createMirCode(MIR_LABEL);
    trueBranchMirCode->mirLabel->label = trueLabel;
    //must have false, if the statement not declared one, then jump over the "true" blocks.
    MirCode *falseBranchMirCode = createMirCode(MIR_LABEL);
    falseBranchMirCode->mirLabel->label = falseLabel;
    char *loopEntryLabel = allocTempLabel();
    //loop entry
    MirCode *loopBranchMirCode = createMirCode(MIR_LABEL);
    loopBranchMirCode->mirLabel->label = loopEntryLabel;
    emitMirCode(loopBranchMirCode);

    generateExpressionBool(
            forStatement->controlExpression,
            trueBranchMirCode->mirLabel,
            falseBranchMirCode->mirLabel
    );
    //true
    emitMirCode(trueBranchMirCode);
    generateStatement(forStatement->statement);
    generateExpression(forStatement->afterExpression, nullptr);
    //go back to loop entry
    MirCode *loopBackEntryMirCode = createMirCode(MIR_JMP);
    loopBackEntryMirCode->mirLabel->label = loopEntryLabel;
    emitMirCode(loopBackEntryMirCode);
    //false
    emitMirCode(falseBranchMirCode);
    //exit block
    optFlag = createMirCode(MIR_OPT_FLAG);
    optFlag->optFlag = OPT_EXIT_LOOP_BLOCK;
    emitMirCode(optFlag);
}

void generateExpression(AstExpression *expression, MirOperand *operand) {
    switch (expression->expressionType) {
        case EXPRESSION_ASSIGNMENT: {
            generateExpressionAssignment(expression->assignmentExpression, operand);
            break;
        }
        case EXPRESSION_ARITHMETIC: {
            //stupid but legal statement like this: a+1; 3*4-1;
            //take no effect
            generateExpressionArithmetic(expression->arithmeticExpression, operand);
            break;
        }
    }
}

void generateStatementExpressions(AstStatementExpressions *astStatementExpressions) {
    //expressions with ";" was wrapper to a statement
    generateExpression(astStatementExpressions->expression, nullptr);
}

void generateStatementDefine(AstStatementDefine *astStatementDefine) {
    //expressions with ";" was wrapper to a statement
    MirCode *mirCode = createMirCode(MIR_2);
    mirCode->mir2->distIdentity = astStatementDefine->identity->name;
    mirCode->mir2->op = OP_ASSIGNMENT;
    generateExpression(astStatementDefine->expression, &mirCode->mir2->fromValue);
    fixMir2Type(mirCode->mir2);
    addVarInfo(mirCode->mir2->distIdentity, mirCode->mir2->distType);
    emitMirCode(mirCode);
}

void generateStatement(AstStatement *astStatement) {
    switch (astStatement->statementType) {
        case STATEMENT_EXPRESSION: {
            AstStatementExpressions *expressionsStatement = astStatement->expressionsStatement;
            generateStatementExpressions(expressionsStatement);
            break;
        }
        case STATEMENT_DEFINE: {
            AstStatementDefine *defineStatement = astStatement->defineStatement;
            generateStatementDefine(defineStatement);
            break;
        }
        case STATEMENT_RETURN: {
            AstStatementReturn *returnStatement = astStatement->returnStatement;
            generateStatementReturn(returnStatement);
            break;
        }
        case STATEMENT_METHOD_CALL: {
            AstStatementMethodCall *methodCallStatement = astStatement->methodCallStatement;
            generateStatementMethodCall(methodCallStatement);
            break;
        }
        case STATEMENT_IF: {
            AstStatementIf *ifStatement = astStatement->ifStatement;
            generateStatementIf(ifStatement);
            break;
        }
        case STATEMENT_WHILE: {
            AstStatementWhile *whileStatement = astStatement->whileStatement;
            generateStatementWhile(whileStatement);
            break;
        }
        case STATEMENT_FOR: {
            AstStatementFor *forStatement = astStatement->forStatement;
            generateStatementFor(forStatement);
            break;
        }
        case STATEMENT_BLOCK: {
            generateStatementSeq(astStatement->blockStatement->statementSeq);
            break;
        }
    }
}

/**
 * complete!
 * @param astStatementSeq
 */
void generateStatementSeq(AstStatementSeq *astStatementSeq) {
    if (astStatementSeq == nullptr) {
        return;
    }
    while (astStatementSeq != nullptr) {
        generateStatement(astStatementSeq->statement);
        astStatementSeq = astStatementSeq->next;
    }
}

/**
 * complete!
 * @param astParamList
 * @return param size
 */
void generateParam(AstParamList *astParamList, MirMethod *mirMethod) {
    AstParamList *curAstParamList = astParamList;
    MirMethodParam *mirMethodParam = nullptr;
    MirMethodParam *firstMirMethodParam = nullptr;
    MirMethodParam *lastMirMethodParam = nullptr;
    while (curAstParamList != nullptr) {
        mirMethodParam = (MirMethodParam *) malloc(sizeof(MirMethodParam));
        mirMethodParam->next = nullptr;
        if (firstMirMethodParam == nullptr) {
            firstMirMethodParam = mirMethodParam;
        }
        if (lastMirMethodParam != nullptr) {
            lastMirMethodParam->next = mirMethodParam;
        }
        lastMirMethodParam = mirMethodParam;
        //real travel ast
        mirMethodParam->paramName = curAstParamList->paramDefine->identity->name;
        mirMethodParam->sign = true;
        switch (curAstParamList->paramDefine->type->primitiveType) {
            case TYPE_CHAR: {
                mirMethodParam->integer = true;
                mirMethodParam->byte = 1;
                break;
            }
            case TYPE_SHORT: {
                mirMethodParam->integer = true;
                mirMethodParam->byte = 2;
                break;
            }
            case TYPE_INT: {
                mirMethodParam->integer = true;
                mirMethodParam->byte = 4;
                break;
            }
            case TYPE_LONG: {
                mirMethodParam->integer = true;
                mirMethodParam->byte = 8;
                break;
            }
            case TYPE_FLOAT: {
                mirMethodParam->integer = false;
                mirMethodParam->byte = 4;
                break;
            }
            case TYPE_DOUBLE: {
                mirMethodParam->integer = false;
                mirMethodParam->byte = 8;
                break;
            }
            case TYPE_UNKNOWN:
            default: {
                loge(MIR_TAG, "unknown type!");
                break;
            }
        }
        addVarInfo(mirMethodParam->paramName,
                   convertAstType2MirType(curAstParamList->paramDefine->type));

        curAstParamList = curAstParamList->next;
    }
    mirMethod->param = firstMirMethodParam;
}


/**
 * complete!
 * @param astMethodDefine
 * @param mirMethod
 */
void generateMethod(AstMethodDefine *astMethodDefine, MirMethod *mirMethod) {
    mirMethod->label = astMethodDefine->identity->name;
//    logd(MIR_TAG, "--- mir method:%s", mirMethod->label);
    addMethodInfo(astMethodDefine->identity->name, convertAstType2MirType(astMethodDefine->type));
    if (astMethodDefine->paramList != nullptr) {
        //var in method params need stack
        generateParam(astMethodDefine->paramList, mirMethod);
    } else {
        mirMethod->param = nullptr;
    }
    AstStatementSeq *astStatementSeq = astMethodDefine->statementBlock->statementSeq;
    //start mir code session
    startMirCodeSession();

    generateStatementSeq(astStatementSeq);

    mirMethod->code = getMirCodeSessionHead();
    finishMirCodeSession();
    //reset temp val pool after method
    resetTempValIndex();
}

/**
 * complete!
 * @param program
 * @return
 */
Mir *generateMir(AstProgram *program) {
    logd(MIR_TAG, "generate mir...");
    Mir *mir = (Mir *) malloc(sizeof(Mir));
    AstMethodSeq *astMethodSeq = program->methodSeq;
    MirMethod *mirMethod = nullptr;
    MirMethod *lastMirMethod = nullptr;
    MirMethod *firstMirMethod = nullptr;
    int methodSize = 0;
    while (astMethodSeq != nullptr) {
        //maintain mir linked-list
        mirMethod = (MirMethod *) malloc(sizeof(MirMethod));
        mirMethod->next = nullptr;
        if (firstMirMethod == nullptr) {
            firstMirMethod = mirMethod;
        }
        if (lastMirMethod != nullptr) {
            lastMirMethod->next = mirMethod;
        }
        lastMirMethod = mirMethod;
        //real travel ast
        methodSize++;
        generateMethod(astMethodSeq->methodDefine, mirMethod);
        astMethodSeq = astMethodSeq->nextAstMethodSeq;
    }
    mir->mirMethod = firstMirMethod;
    mir->methodSize = methodSize;
    return mir;
}