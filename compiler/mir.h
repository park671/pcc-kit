//
// Created by Park Yu on 2024/9/23.
//

#ifndef PCC_CC_MIR_H
#define PCC_CC_MIR_H

#include "ast.h"
#include <stdint.h>

struct MirMethodParam;
struct MirMethod;

struct MirData;

struct Mir {
    int methodSize = 0;
    MirMethod *mirMethod;
    int dataSize = 0;
    MirData *mirData;
};

enum MirType {
    MIR_3,
    MIR_2,
    MIR_CMP,
    MIR_JMP,
    MIR_CALL,
    MIR_RET,
    MIR_LABEL,

    //for optimization
    MIR_OPT_FLAG,
};

struct MirCode;

struct MirMethod {
    char *label;//identity, in another word: method name.
    MirMethodParam *param;//nullable
    MirCode *code;

    MirMethod *next;
};

struct MirMethodParam {
    char *paramName;
    bool integer;//true-int, false-float
    bool sign;//true-signed, false-unsigned
    int byte;//1-int8, 2-int16, 4-int32, 8-int64
    MirMethodParam *next;
};

enum MirOptFlag {
    OPT_ENTER_BLOCK,
    OPT_EXIT_BLOCK,
    OPT_ENTER_LOOP_BLOCK,
    OPT_EXIT_LOOP_BLOCK,
};

struct Mir3;
struct Mir2;
struct MirCmp;
struct MirCall;
struct MirRet;
struct MirLabel;

struct MirCode {
    MirType mirType;
    int codeLine;
    union {
        Mir2 *mir2;
        Mir3 *mir3;
        MirCmp *mirCmp;
        MirCall *mirCall;
        MirRet *mirRet;
        MirLabel *mirLabel;
        MirOptFlag optFlag;
    };
    MirCode *nextCode;
};

enum MirOperandType {
    OPERAND_UNKNOWN,
    OPERAND_IDENTITY,
    OPERAND_RET,

    OPERAND_VOID,

    OPERAND_INT8,
    OPERAND_INT16,
    OPERAND_INT32,
    OPERAND_INT64,

    OPERAND_FLOAT32,
    OPERAND_FLOAT64
};

struct MirOperand {
    MirOperandType type;
    union {
        char *identity;

        int8_t dataInt8;
        int16_t dataInt16;
        int32_t dataInt32;
        int64_t dataInt64;

        float dataFloat32;
        double dataFloat64;
    };
};

enum MirOperator {
    //arithmetic
    OP_UNKNOWN,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_ASSIGNMENT,
};

struct Mir3 {
    MirOperandType distType;
    char *distIdentity;
    MirOperand value1;
    MirOperator op;
    MirOperand value2;
};

struct Mir2 {
    MirOperandType distType;
    char *distIdentity;
    MirOperator op;
    MirOperand fromValue;
};

enum MirBooleanOperator {
    //boolean
    CMP_UNKNOWN,
    CMP_G,
    CMP_L,
    CMP_GE,
    CMP_LE,
    CMP_E,
    CMP_NE,
};

struct MirCmp {
    MirOperand value1;
    MirBooleanOperator op;
    MirOperand value2;
    MirLabel *trueLabel;//not null
    MirLabel *falseLabel;//nullable
};

struct MirRet {
    MirOperand *value;//nullable
};

struct MirLabel {
    char *label;
};

struct MirObjectList {
    MirOperand value;
    MirObjectList *next;//nullable
};

struct MirCall {
    char *label;
    MirObjectList *mirObjectList;//nullable
};

struct MirData {
    //todo
    MirData *next;
};

extern Mir *generateMir(AstProgram *program);

extern void printMirCode(MirCode *mirCode);

#endif