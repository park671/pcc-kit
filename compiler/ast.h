//
// Created by Park Yu on 2024/9/11.
//

#ifndef PCC_CC_AST_H
#define PCC_CC_AST_H

#include "stdint.h"

enum PrimitiveType {
    TYPE_UNKNOWN,

    TYPE_VOID,

    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,

    TYPE_FLOAT,
    TYPE_DOUBLE,
};

enum StatementType {
    STATEMENT_IF,
    STATEMENT_WHILE,
    STATEMENT_FOR,
    STATEMENT_RETURN,
    STATEMENT_BLOCK,
    STATEMENT_DEFINE,
    STATEMENT_EXPRESSION,
    STATEMENT_METHOD_CALL,
};

enum BoolOperatorType {
    BOOL_NULL,
    BOOL_OR,
    BOOL_AND
};

enum ArithmeticOperatorType {
    ARITHMETIC_NULL,
    ARITHMETIC_ADD,
    ARITHMETIC_SUB,
    ARITHMETIC_MUL,
    ARITHMETIC_DIV,
    ARITHMETIC_MOD,
};

enum RelationOperator {
    RELATION_LESS,
    RELATION_LESS_EQ,
    RELATION_GREATER,
    RELATION_GREATER_EQ,
    RELATION_EQ,
    RELATION_NOT_EQ,
};

enum BoolFactorType {
    BOOL_FACTOR_INVERT,
    BOOL_FACTOR_RELATION,
};

enum ArithmeticFactorType {
    ARITHMETIC_IDENTITY,
    ARITHMETIC_METHOD_RET,//函数的返回值
    ARITHMETIC_CHAR,
    ARITHMETIC_INT,
    ARITHMETIC_SHORT,
    ARITHMETIC_LONG,
    ARITHMETIC_FLOAT,
    ARITHMETIC_DOUBLE,
};

enum ExpressionType {
    EXPRESSION_ASSIGNMENT,
    EXPRESSION_ARITHMETIC,
    EXPRESSION_POINTER,
};

struct AstType {
    bool isPointer;
    PrimitiveType primitiveType;
};

enum IdentityType {
    ID_METHOD,
    ID_VAR,
    ID_ARRAY,
};

struct AstIdentity {
    IdentityType type;
    const char *name;
};

struct AstArray {
    PrimitiveType primitiveType;
    int size;
    void *buffer;
};

struct AstParamDefine {
    AstType *type;
    AstIdentity *identity;
};

struct AstParamList {
    //maybe ","
    AstParamDefine *paramDefine;
    AstParamList *next;
    //null
};

struct AstBoolFactor;

struct AstBoolFactorInvert {
    // !
    AstBoolFactor *boolFactor;
};

struct AstExpressionArithmetic;

struct AstBoolFactorCompareArithmetic {
    AstExpressionArithmetic *firstArithmeticExpression;
    RelationOperator relationOperation;
    AstExpressionArithmetic *secondArithmeticExpression;
};

struct AstBoolFactor {
    BoolFactorType boolFactorType;
    union {
        AstBoolFactorInvert *invertBoolFactor;
        AstBoolFactorCompareArithmetic *arithmeticBoolFactor;
    };
};

struct AstBoolItem {
    AstBoolFactor *boolFactor;
    AstBoolItem *next;//nullable, must be &&
};

struct AstExpressionBool {
    AstBoolItem *boolItem;
    AstExpressionBool *next;//nullable, must be ||
};

struct AstStatementMethodCall;

struct AstArithmeticFactor {
    ArithmeticFactorType factorType;
    union {
        AstIdentity *identity;
        AstStatementMethodCall *methodCall;
        char dataChar;
        short dataShort;
        int dataInt;
        long dataLong;

        float dataFloat;
        double dataDouble;
    };
};

struct AstArithmeticItemMore {
    ArithmeticOperatorType arithmeticOperatorType;//* / %
    AstArithmeticFactor *arithmeticFactor;
    AstArithmeticItemMore *arithmeticItemMore;
    //null
};

struct AstArithmeticItem {
    AstArithmeticFactor *arithmeticFactor;
    AstArithmeticItemMore *arithmeticItemMore;
};

struct AstExpressionArithmeticMore {
    ArithmeticOperatorType arithmeticOperatorType;//+ -
    AstArithmeticItem *arithmeticItem;
    AstExpressionArithmeticMore *arithmeticExpressMore;
    //null
};

struct AstExpressionArithmetic {
    AstArithmeticItem *arithmeticItem;
    AstExpressionArithmeticMore *arithmeticExpressMore;
};

struct AstExpression;

enum ExpressionPointerType {
    EXP_POINTER_CALC,
    EXP_POINTER_ARRAY,
};

struct AstExpressionPointerCalc {
    //&
    AstIdentity *identity;
};

struct AstExpressionPointerArray {
    AstArray *array;
};

struct AstExpressionPointer {
    ExpressionPointerType type;
    union {
        AstExpressionPointerCalc *pointerCalc;
        AstExpressionPointerArray *pointerArray;
    };
};

struct AstExpressionAssignment {
    AstIdentity *identity;
    //=
    AstExpression *expression;
};

struct AstExpression {
    ExpressionType expressionType;
    union {
        AstExpressionAssignment *assignmentExpression;
        AstExpressionArithmetic *arithmeticExpression;
        AstExpressionPointer *pointerExpression;
    };
};

struct AstObjectList {
    //maybe ","
    AstExpression *expression;
    AstObjectList *objectMore;
    //null
};

struct AstStatementDefine {
    AstType *type;
    AstIdentity *identity;
    //=
    AstExpression *expression;
};

struct AstStatementMethodCall {
    AstType *retType;
    AstIdentity *identity;
    //(
    AstObjectList *objectList;
    //)
    //;
};

struct AstStatement;

struct AstStatementFor {
    //for
    //(
    AstExpression *initExpression;
    //;
    AstExpressionBool *controlExpression;
    //;
    AstExpression *afterExpression;
    //)
    AstStatement *statement;
};

struct AstStatementWhile {
    //while
    //(
    AstExpressionBool *expression;
    //)
    AstStatement *statement;
};

struct AstStatementIf {
    //if
    //(
    AstExpressionBool *expression;
    //)
    AstStatement *trueStatement;
    //else
    AstStatement *falseStatement;//nullable
};

struct AstStatementReturn {
    //return
    AstExpression *expression;//nullable
};

struct AstStatementExpressions {
    AstExpression *expression;
};

struct AstStatementSeq;
struct AstStatementBlock;

struct AstStatement {
    StatementType statementType;
    union {
        AstStatementDefine *defineStatement;
        AstStatementExpressions *expressionsStatement;
        AstStatementMethodCall *methodCallStatement;
        AstStatementIf *ifStatement;
        AstStatementFor *forStatement;
        AstStatementReturn *returnStatement;
        AstStatementWhile *whileStatement;
        AstStatementBlock *blockStatement;
    };
};

struct AstStatementSeq {
    AstStatement *statement;
    AstStatementSeq *next;
    //null
};

struct AstStatementBlock {
    //{
    AstStatementSeq *statementSeq;
    //}
};

struct AstMethodDefine {
    AstType *type;
    AstIdentity *identity;
    // (
    AstParamList *paramList;
    //)
    AstStatementBlock *statementBlock;
};

struct AstMethodSeq {
    AstMethodDefine *methodDefine;
    AstMethodSeq *nextAstMethodSeq;
    //null
};

struct AstGlobalFieldSeq {
    AstStatementDefine *statementDefine;
    AstGlobalFieldSeq *next;
    //null
};

struct AstProgram {
    AstGlobalFieldSeq *globalFieldSeq;
    AstMethodSeq *methodSeq;
};

enum AstNodeType {
    NODE_PROGRAM,
    NODE_GLOBAL_FIELD_SEQ,
    NODE_METHOD_SEQ,
    NODE_METHOD_DEFINE,
    NODE_STATEMENT_BLOCK,
    NODE_STATEMENT_SEQ,
    NODE_STATEMENT,
    NODE_STATEMENT_EXPRESSIONS,
    NODE_STATEMENT_IF,
    NODE_STATEMENT_WHILE,
    NODE_STATEMENT_FOR,
    NODE_STATEMENT_RETURN,
    NODE_STATEMENT_METHOD_CALL,
    NODE_OBJECT_LIST,
    NODE_EXPRESSION,
    NODE_EXPRESSION_POINTER,
    NODE_EXPRESSION_ASSIGNMENT,
    NODE_EXPRESSION_ARITHMETIC,
    NODE_EXPRESSION_ARITHMETIC_MORE,
    NODE_ARITHMETIC_ITEM,
    NODE_ARITHMETIC_ITEM_MORE,
    NODE_ARITHMETIC_FACTOR,
    NODE_EXPRESSION_BOOL,
    NODE_BOOL_ITEM,
    NODE_BOOL_FACTOR,
    NODE_BOOL_FACTOR_COMPARE_ARITHMETIC,
    NODE_BOOL_FACTOR_INVERT,
    NODE_PARAM_LIST,
    NODE_PARAM_DEFINE,
    NODE_IDENTITY,
    NODE_TYPE
};

#endif //PCC_CC_AST_H
