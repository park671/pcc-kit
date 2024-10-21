//
// Created by Park Yu on 2024/9/11.
//

#include <stack>
#include <vector>
#include "syntaxer.h"
#include "logger.h"
#include "string.h"
#include "mspace.h"

using namespace std;

//16KB
#define BUFFER_SIZE 16384
#define SYNTAX_TAG "syntaxer"

static stack<vector<AstIdentity *> *> varList;
static vector<AstMethodDefine *> methodList;

inline static bool hasVarDefine(const char *name) {
    vector<AstIdentity *> *currentMethodStack = varList.top();
    for (int i = 0; i < currentMethodStack->size(); i++) {
        if (strcmp((*currentMethodStack)[i]->name, name) == 0) {
            return true;
        }
    }
    return false;
}

inline static bool hasMethodDefine(const char *name) {
    for (int i = 0; i < methodList.size(); i++) {
        if (strcmp(methodList[i]->identity->name, name) == 0) {
            return true;
        }
    }
    return false;
}

inline static AstMethodDefine *getMethodDefine(const char *name) {
    for (int i = 0; i < methodList.size(); i++) {
        if (strcmp(methodList[i]->identity->name, name) == 0) {
            return methodList[i];
        }
    }
    return nullptr;
}

inline static void pushMethod(AstMethodDefine *astMethodDefine) {
    methodList.push_back(astMethodDefine);
    vector<AstIdentity *> *methodStack = new vector<AstIdentity *>();
    varList.push(methodStack);
}

inline static void popMethod() {
    vector<AstIdentity *> *methodStack = varList.top();
    delete methodStack;
    varList.pop();
}

inline static void addVar(AstIdentity *astIdentity) {
    varList.top()->push_back(astIdentity);
}

inline static PrimitiveType convertTokenType2PrimitiveType(const char *type) {
    if (strcmp(type, "char") == 0) {
        return TYPE_CHAR;
    } else if (strcmp(type, "short") == 0) {
        return TYPE_SHORT;
    } else if (strcmp(type, "int") == 0) {
        return TYPE_INT;
    } else if (strcmp(type, "long") == 0) {
        return TYPE_LONG;
    } else if (strcmp(type, "float") == 0) {
        return TYPE_FLOAT;
    } else if (strcmp(type, "double") == 0) {
        return TYPE_DOUBLE;
    } else if (strcmp(type, "void") == 0) {
        return TYPE_VOID;
    } else {
        loge(SYNTAX_TAG, "[-]error: unknown type: %s", type);
        return TYPE_UNKNOWN;
    }
}

Token *travelAst(Token *token, void *currentNode, AstNodeType nodeType) {
    switch (nodeType) {
        case NODE_PROGRAM: {
            AstProgram *astProgram = (AstProgram *) currentNode;
            if (token == nullptr) {
                astProgram->methodSeq = nullptr;
            } else {
                astProgram->methodSeq = (AstMethodSeq *) pccMalloc(SYNTAX_TAG, sizeof(AstMethodSeq));
                token = travelAst(token, astProgram->methodSeq, NODE_METHOD_SEQ);
            }
            break;
        }
        case NODE_METHOD_SEQ: {
            AstMethodSeq *astMethodSeq = (AstMethodSeq *) currentNode;
            astMethodSeq->methodDefine = (AstMethodDefine *) pccMalloc(SYNTAX_TAG, sizeof(AstMethodDefine));
            token = travelAst(token, astMethodSeq->methodDefine, NODE_METHOD_DEFINE);
            if (token != nullptr) {
                astMethodSeq->nextAstMethodSeq = (AstMethodSeq *) pccMalloc(SYNTAX_TAG, sizeof(AstMethodSeq));
                token = travelAst(token, astMethodSeq->nextAstMethodSeq, NODE_METHOD_SEQ);
            }
            break;
        }
        case NODE_METHOD_DEFINE: {
            AstMethodDefine *astMethodDefine = (AstMethodDefine *) currentNode;
            //method type
            if (token->tokenType != TOKEN_TYPE) {
                loge(SYNTAX_TAG, "[-]error: method define need type: %s", token->content);
                return nullptr;
            }
            astMethodDefine->type = (AstType *) pccMalloc(SYNTAX_TAG, sizeof(AstType));
            astMethodDefine->type->primitiveType = convertTokenType2PrimitiveType(token->content);
            token = token->next;
            //method name
            if (token->tokenType != TOKEN_IDENTIFIER) {
                loge(SYNTAX_TAG, "[-]error: method define need identifier: %s", token->content);
                return nullptr;
            }
            astMethodDefine->identity = (AstIdentity *) pccMalloc(SYNTAX_TAG, sizeof(AstIdentity));
            astMethodDefine->identity->name = token->content;
            token = token->next;
            //method (
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "(") != 0) {
                loge(SYNTAX_TAG, "[-]error: method define need (: %s", token->content);
                return nullptr;
            }
            token = token->next;
            pushMethod(astMethodDefine);
            //method param
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ")") == 0) {
                astMethodDefine->paramList = nullptr;
            } else {
                astMethodDefine->paramList = (AstParamList *) pccMalloc(SYNTAX_TAG, sizeof(AstParamList));
                token = travelAst(token, astMethodDefine->paramList, NODE_PARAM_LIST);
            }
            //method )
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ")") != 0) {
                loge(SYNTAX_TAG, "[-]error: method define need ): %s", token->content);
                return nullptr;
            }
            token = token->next;
            //method code block
            astMethodDefine->statementBlock = (AstStatementBlock *) pccMalloc(SYNTAX_TAG, sizeof(AstStatementBlock));
            token = travelAst(token, astMethodDefine->statementBlock, NODE_STATEMENT_BLOCK);
            popMethod();
            break;
        }
        case NODE_PARAM_LIST: {
            AstParamList *astParamList = (AstParamList *) currentNode;
            astParamList->paramDefine = (AstParamDefine *) pccMalloc(SYNTAX_TAG, sizeof(AstParamDefine));
            token = travelAst(token, astParamList->paramDefine, NODE_PARAM_DEFINE);
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ",") == 0) {
                //consume ","
                token = token->next;
                astParamList->next = (AstParamList *) pccMalloc(SYNTAX_TAG, sizeof(AstParamList));
                token = travelAst(token, astParamList->next, NODE_PARAM_LIST);
            } else {
                astParamList->next = nullptr;
            }
            break;
        }
        case NODE_PARAM_DEFINE: {
            AstParamDefine *astParamDefine = (AstParamDefine *) currentNode;
            if (token->tokenType != TOKEN_TYPE) {
                loge(SYNTAX_TAG, "[-]error: param define need type: %s", token->content);
                return nullptr;
            }
            astParamDefine->type = (AstType *) pccMalloc(SYNTAX_TAG, sizeof(AstType));
            astParamDefine->type->primitiveType = convertTokenType2PrimitiveType(token->content);
            token = token->next;
            if (token->tokenType != TOKEN_IDENTIFIER) {
                loge(SYNTAX_TAG, "[-]error: param define need identifier: %s", token->content);
                return nullptr;
            }
            astParamDefine->identity = (AstIdentity *) pccMalloc(SYNTAX_TAG, sizeof(AstIdentity));
            astParamDefine->identity->name = token->content;
            addVar(astParamDefine->identity);
            token = token->next;
            break;
        }
        case NODE_STATEMENT_BLOCK: {
            AstStatementBlock *astStatementBlock = (AstStatementBlock *) currentNode;
            //block {
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "{") != 0) {
                loge(SYNTAX_TAG, "[-]error: code block define need {: %s", token->content);
                return nullptr;
            }
            token = token->next;
            //statement seq
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, "}") == 0) {
                astStatementBlock->statementSeq = nullptr;
            } else {
                astStatementBlock->statementSeq = (AstStatementSeq *) pccMalloc(SYNTAX_TAG, sizeof(AstStatementSeq));
                token = travelAst(token, astStatementBlock->statementSeq, NODE_STATEMENT_SEQ);
            }
            //block }
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "}") != 0) {
                loge(SYNTAX_TAG, "[-]error: code block define need }: %s", token->content);
                return nullptr;
            }
            token = token->next;
            break;
        }
        case NODE_STATEMENT_SEQ: {
            AstStatementSeq *astStatementSeq = (AstStatementSeq *) currentNode;
            astStatementSeq->statement = (AstStatement *) pccMalloc(SYNTAX_TAG, sizeof(AstStatement));
            token = travelAst(token, astStatementSeq->statement, NODE_STATEMENT);
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, "}") == 0) {
                astStatementSeq->next = nullptr;
            } else {
                astStatementSeq->next = (AstStatementSeq *) pccMalloc(SYNTAX_TAG, sizeof(AstStatementSeq));
                token = travelAst(token, astStatementSeq->next, NODE_STATEMENT_SEQ);
            }
            break;
        }
        case NODE_STATEMENT: {
            AstStatement *astStatement = (AstStatement *) currentNode;
            if (token->tokenType == TOKEN_KEYWORD) {
                if (strcmp(token->content, "if") == 0) {
                    astStatement->statementType = STATEMENT_IF;
                    astStatement->ifStatement = (AstStatementIf *) pccMalloc(SYNTAX_TAG, sizeof(AstStatementIf));
                    //consume if
                    token = token->next;
                    token = travelAst(token, astStatement->ifStatement, NODE_STATEMENT_IF);
                } else if (strcmp(token->content, "while") == 0) {
                    astStatement->statementType = STATEMENT_WHILE;
                    astStatement->whileStatement = (AstStatementWhile *) pccMalloc(SYNTAX_TAG,
                                                                                   sizeof(AstStatementWhile));
                    //consume while
                    token = token->next;
                    token = travelAst(token, astStatement->whileStatement, NODE_STATEMENT_WHILE);
                } else if (strcmp(token->content, "for") == 0) {
                    astStatement->statementType = STATEMENT_FOR;
                    astStatement->forStatement = (AstStatementFor *) pccMalloc(SYNTAX_TAG, sizeof(AstStatementFor));
                    //consume for
                    token = token->next;
                    token = travelAst(token, astStatement->forStatement, NODE_STATEMENT_FOR);
                } else if (strcmp(token->content, "return") == 0) {
                    astStatement->statementType = STATEMENT_RETURN;
                    astStatement->returnStatement = (AstStatementReturn *) pccMalloc(SYNTAX_TAG,
                                                                                     sizeof(AstStatementReturn));
                    //consume return
                    token = token->next;
                    token = travelAst(token, astStatement->forStatement, NODE_STATEMENT_RETURN);
                }
            } else if (token->tokenType == TOKEN_TYPE) {
                astStatement->statementType = STATEMENT_DEFINE;
                astStatement->defineStatement = (AstStatementDefine *) pccMalloc(SYNTAX_TAG,
                                                                                 sizeof(AstStatementDefine));
                astStatement->defineStatement->type = (AstType *) pccMalloc(SYNTAX_TAG, sizeof(AstType));
                astStatement->defineStatement->type->primitiveType = convertTokenType2PrimitiveType(token->content);
                token = token->next;
                if (token->tokenType != TOKEN_IDENTIFIER) {
                    loge(SYNTAX_TAG, "[-]error: var define need identifier: %s", token->content);
                    return nullptr;
                }
                astStatement->defineStatement->identity = (AstIdentity *) pccMalloc(SYNTAX_TAG, sizeof(AstIdentity));
                astStatement->defineStatement->identity->name = token->content;
                //record
                addVar(astStatement->defineStatement->identity);
                token = token->next;
                if (token->tokenType != TOKEN_OPERATOR || strcmp(token->content, "=") != 0) {
                    loge(SYNTAX_TAG, "[-]error: var define need init fromValue: %s", token->content);
                    return nullptr;
                }
                //consume =
                token = token->next;
                astStatement->defineStatement->expression = (AstExpression *) pccMalloc(SYNTAX_TAG,
                                                                                        sizeof(AstExpression));
                token = travelAst(token, astStatement->defineStatement->expression, NODE_EXPRESSION);
                if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ";") != 0) {
                    loge(SYNTAX_TAG, "[-]error: need ;: %s", token->content);
                    exit(1);
                }
                //consume ;
                token = token->next;
            } else {
                if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, "{") == 0) {
                    //do not consume {, left it to block statement
                    astStatement->statementType = STATEMENT_BLOCK;
                    astStatement->blockStatement = (AstStatementBlock *) pccMalloc(SYNTAX_TAG,
                                                                                   sizeof(AstStatementBlock));
                    token = travelAst(token, astStatement->blockStatement, NODE_STATEMENT_BLOCK);
                } else if (token->tokenType == TOKEN_IDENTIFIER
                           && token->next != nullptr
                           && token->next->tokenType == TOKEN_BOUNDARY
                           && strcmp(token->next->content, "(") == 0
                        ) {
                    //this is method call
                    if (hasMethodDefine(token->content)) {
                        astStatement->statementType = STATEMENT_METHOD_CALL;
                        astStatement->methodCallStatement = (AstStatementMethodCall *) pccMalloc(SYNTAX_TAG,
                                                                                                 sizeof(AstStatementMethodCall));
                        token = travelAst(token, astStatement->methodCallStatement, NODE_STATEMENT_METHOD_CALL);
                    } else {
                        loge(SYNTAX_TAG, "[-]error: undefined method: %s", token->content);
                        exit(1);
                    }
                } else {
                    //assume this is expressions statement
                    astStatement->statementType = STATEMENT_EXPRESSION;
                    astStatement->expressionsStatement = (AstStatementExpressions *) pccMalloc(SYNTAX_TAG,
                                                                                               sizeof(AstStatementExpressions));
                    token = travelAst(token, astStatement->expressionsStatement, NODE_STATEMENT_EXPRESSIONS);
                }
            }
            break;
        }
        case NODE_STATEMENT_EXPRESSIONS: {
            AstStatementExpressions *astStatementExpressions = (AstStatementExpressions *) currentNode;
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ";") == 0) {
                astStatementExpressions->expression = nullptr;
            } else {
                astStatementExpressions->expression = (AstExpression *) pccMalloc(SYNTAX_TAG, sizeof(AstExpression));
                token = travelAst(token, astStatementExpressions->expression, NODE_EXPRESSION);
            }
            //consume ;
            token = token->next;
            break;
        }
        case NODE_EXPRESSION: {
            AstExpression *astExpression = (AstExpression *) currentNode;
            if (token->tokenType == TOKEN_IDENTIFIER
                && token->next != nullptr
                && token->next->tokenType == TOKEN_OPERATOR
                && strcmp(token->next->content, "=") == 0) {
                //assignment
                if (hasVarDefine(token->content)) {
                    astExpression->expressionType = EXPRESSION_ASSIGNMENT;
                    astExpression->assignmentExpression = (AstExpressionAssignment *) pccMalloc(SYNTAX_TAG,
                                                                                                sizeof(AstExpressionAssignment));
                    token = travelAst(token, astExpression->assignmentExpression, NODE_EXPRESSION_ASSIGNMENT);
                } else {
                    loge(SYNTAX_TAG, "[-]error: undefined var: %s", token->content);
                }
                break;
            } else {
                astExpression->expressionType = EXPRESSION_ARITHMETIC;
                astExpression->arithmeticExpression = (AstExpressionArithmetic *) pccMalloc(SYNTAX_TAG,
                                                                                            sizeof(AstExpressionArithmetic));
                token = travelAst(token, astExpression->arithmeticExpression, NODE_EXPRESSION_ARITHMETIC);
            }
            break;
        }
        case NODE_EXPRESSION_ASSIGNMENT: {
            AstExpressionAssignment *astExpressionAssignment = (AstExpressionAssignment *) currentNode;
            astExpressionAssignment->identity = (AstIdentity *) pccMalloc(SYNTAX_TAG, sizeof(AstIdentity));
            astExpressionAssignment->identity->name = token->content;
            //consume identity
            token = token->next;
            if (token->tokenType != TOKEN_OPERATOR || strcmp(token->content, "=") != 0) {
                loge(SYNTAX_TAG, "[-]error: var assignment need fromValue: %s", token->content);
                return nullptr;
            }
            //consume =
            token = token->next;
            astExpressionAssignment->expression = (AstExpression *) pccMalloc(SYNTAX_TAG,
                                                                              sizeof(AstExpression));
            token = travelAst(token, astExpressionAssignment->expression,
                              NODE_EXPRESSION);
            break;
        }
        case NODE_OBJECT_LIST: {
            AstObjectList *astObjectList = (AstObjectList *) currentNode;
            astObjectList->expression = (AstExpression *) pccMalloc(SYNTAX_TAG, sizeof(AstExpression));
            token = travelAst(token, astObjectList->expression, NODE_EXPRESSION);
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ",") == 0) {
                //consume ,
                token = token->next;
                astObjectList->objectMore = (AstObjectList *) pccMalloc(SYNTAX_TAG, sizeof(AstObjectList));
                token = travelAst(token, astObjectList->objectMore, NODE_OBJECT_LIST);
            }
            break;
        }
        case NODE_STATEMENT_IF: {
            AstStatementIf *astStatementIf = (AstStatementIf *) currentNode;
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "(") != 0) {
                loge(SYNTAX_TAG, "[-]error: need (: %s", token->content);
                return nullptr;
            }
            //consume (
            token = token->next;
            astStatementIf->expression = (AstExpressionBool *) pccMalloc(SYNTAX_TAG, sizeof(AstExpressionBool));
            token = travelAst(token, astStatementIf->expression, NODE_EXPRESSION_BOOL);
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ")") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ): %s", token->content);
                return nullptr;
            }
            //consume )
            token = token->next;
            astStatementIf->trueStatement = (AstStatement *) pccMalloc(SYNTAX_TAG, sizeof(AstStatement));
            token = travelAst(token, astStatementIf->trueStatement, NODE_STATEMENT);
            if (token->tokenType != TOKEN_KEYWORD || strcmp(token->content, "else") != 0) {
                astStatementIf->falseStatement = nullptr;
            } else {
                //consume else
                token = token->next;
                astStatementIf->falseStatement = (AstStatement *) pccMalloc(SYNTAX_TAG, sizeof(AstStatement));
                token = travelAst(token, astStatementIf->falseStatement, NODE_STATEMENT);
            }
            break;
        }
        case NODE_STATEMENT_WHILE: {
            AstStatementWhile *astStatementWhile = (AstStatementWhile *) currentNode;
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "(") != 0) {
                loge(SYNTAX_TAG, "[-]error: need (: %s", token->content);
                return nullptr;
            }
            //consume (
            token = token->next;
            astStatementWhile->expression = (AstExpressionBool *) pccMalloc(SYNTAX_TAG, sizeof(AstExpressionBool));
            token = travelAst(token, astStatementWhile->expression, NODE_EXPRESSION_BOOL);
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ")") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ): %s", token->content);
                return nullptr;
            }
            //consume )
            token = token->next;
            astStatementWhile->statement = (AstStatement *) pccMalloc(SYNTAX_TAG, sizeof(AstStatement));
            token = travelAst(token, astStatementWhile->statement, NODE_STATEMENT);
            break;
        }
        case NODE_STATEMENT_FOR: {
            AstStatementFor *astStatementFor = (AstStatementFor *) currentNode;
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "(") != 0) {
                loge(SYNTAX_TAG, "[-]error: need (: %s", token->content);
                return nullptr;
            }
            //consume (
            token = token->next;
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ";") == 0) {
                astStatementFor->initExpression = nullptr;
            } else {
                astStatementFor->initExpression = (AstExpression *) pccMalloc(SYNTAX_TAG, sizeof(AstExpression));
                token = travelAst(token, astStatementFor->initExpression, NODE_EXPRESSION);
            }
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ";") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ;: %s", token->content);
                return nullptr;
            }
            //consume ;
            token = token->next;
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ";") == 0) {
                astStatementFor->controlExpression = nullptr;
            } else {
                astStatementFor->controlExpression = (AstExpressionBool *) pccMalloc(SYNTAX_TAG,
                                                                                     sizeof(AstExpressionBool));
                token = travelAst(token, astStatementFor->controlExpression, NODE_EXPRESSION_BOOL);
            }
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ";") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ;: %s", token->content);
                return nullptr;
            }
            //consume ;
            token = token->next;
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ")") == 0) {
                astStatementFor->afterExpression = nullptr;
            } else {
                astStatementFor->afterExpression = (AstExpression *) pccMalloc(SYNTAX_TAG, sizeof(AstExpression));
                token = travelAst(token, astStatementFor->afterExpression, NODE_EXPRESSION);
            }
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ")") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ): %s", token->content);
                return nullptr;
            }
            //consume )
            token = token->next;
            astStatementFor->statement = (AstStatement *) pccMalloc(SYNTAX_TAG, sizeof(AstStatement));
            token = travelAst(token, astStatementFor->statement, NODE_STATEMENT);
            break;
        }
        case NODE_STATEMENT_RETURN: {
            AstStatementReturn *astStatementReturn = (AstStatementReturn *) currentNode;
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ";") == 0) {
                //void return
                //todo check return type with method signature
                astStatementReturn->expression = nullptr;
            } else {
                astStatementReturn->expression = (AstExpression *) pccMalloc(SYNTAX_TAG, sizeof(AstExpression));
                token = travelAst(token, astStatementReturn->expression, NODE_EXPRESSION);
            }
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ";") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ;: %s", token->content);
                return nullptr;
            }
            //consume ;
            token = token->next;
            break;
        }
        case NODE_EXPRESSION_ARITHMETIC: {
            AstExpressionArithmetic *astExpressionArithmetic = (AstExpressionArithmetic *) currentNode;
            astExpressionArithmetic->arithmeticItem = (AstArithmeticItem *) pccMalloc(SYNTAX_TAG,
                                                                                      sizeof(AstArithmeticItem));
            token = travelAst(token, astExpressionArithmetic->arithmeticItem, NODE_ARITHMETIC_ITEM);
            if (token->tokenType == TOKEN_OPERATOR) {
                if (strcmp(token->content, "+") == 0 || strcmp(token->content, "-") == 0) {
                    astExpressionArithmetic->arithmeticExpressMore = (AstExpressionArithmeticMore *) pccMalloc(
                            SYNTAX_TAG,
                            sizeof(AstExpressionArithmeticMore));
                    token = travelAst(token, astExpressionArithmetic->arithmeticExpressMore,
                                      NODE_EXPRESSION_ARITHMETIC_MORE);
                } else {
                    astExpressionArithmetic->arithmeticExpressMore = nullptr;
                }
            } else {
                astExpressionArithmetic->arithmeticExpressMore = nullptr;
            }

            break;
        }
        case NODE_EXPRESSION_ARITHMETIC_MORE: {
            AstExpressionArithmeticMore *astExpressionArithmeticMore = (AstExpressionArithmeticMore *) currentNode;
            if (strcmp(token->content, "+") == 0) {
                astExpressionArithmeticMore->arithmeticOperatorType = ARITHMETIC_ADD;
            } else if (strcmp(token->content, "-") == 0) {
                astExpressionArithmeticMore->arithmeticOperatorType = ARITHMETIC_SUB;
            }
            //consume + -
            token = token->next;
            astExpressionArithmeticMore->arithmeticItem = (AstArithmeticItem *) pccMalloc(SYNTAX_TAG,
                                                                                          sizeof(AstArithmeticItem));
            token = travelAst(token, astExpressionArithmeticMore->arithmeticItem, NODE_ARITHMETIC_ITEM);
            if (strcmp(token->content, "+") == 0 || strcmp(token->content, "-") == 0) {
                astExpressionArithmeticMore->arithmeticExpressMore = (AstExpressionArithmeticMore *) pccMalloc(
                        SYNTAX_TAG,
                        sizeof(AstExpressionArithmeticMore));
                token = travelAst(token, astExpressionArithmeticMore->arithmeticExpressMore,
                                  NODE_EXPRESSION_ARITHMETIC_MORE);
            } else {
                astExpressionArithmeticMore->arithmeticExpressMore = nullptr;
            }
            break;
        }
        case NODE_ARITHMETIC_ITEM: {
            AstArithmeticItem *astArithmeticItem = (AstArithmeticItem *) currentNode;
            astArithmeticItem->arithmeticFactor = (AstArithmeticFactor *) pccMalloc(SYNTAX_TAG,
                                                                                    sizeof(AstArithmeticFactor));
            token = travelAst(token, astArithmeticItem->arithmeticFactor, NODE_ARITHMETIC_FACTOR);
            if (token->tokenType == TOKEN_OPERATOR) {
                if (strcmp(token->content, "*") == 0
                    || strcmp(token->content, "/") == 0
                    || strcmp(token->content, "%") == 0) {
                    astArithmeticItem->arithmeticItemMore = (AstArithmeticItemMore *) pccMalloc(SYNTAX_TAG,
                                                                                                sizeof(AstArithmeticItemMore));
                    token = travelAst(token, astArithmeticItem->arithmeticItemMore,
                                      NODE_ARITHMETIC_ITEM_MORE);
                } else {
                    astArithmeticItem->arithmeticItemMore = nullptr;
                }
            } else {
                astArithmeticItem->arithmeticItemMore = nullptr;
            }
            break;
        }
        case NODE_ARITHMETIC_ITEM_MORE: {
            AstArithmeticItemMore *astArithmeticItemMore = (AstArithmeticItemMore *) currentNode;
            if (strcmp(token->content, "*") == 0) {
                astArithmeticItemMore->arithmeticOperatorType = ARITHMETIC_MUL;
            } else if (strcmp(token->content, "/") == 0) {
                astArithmeticItemMore->arithmeticOperatorType = ARITHMETIC_DIV;
            } else if (strcmp(token->content, "%") == 0) {
                astArithmeticItemMore->arithmeticOperatorType = ARITHMETIC_MOD;
            }
            //consume * / %
            token = token->next;
            astArithmeticItemMore->arithmeticFactor = (AstArithmeticFactor *) pccMalloc(SYNTAX_TAG,
                                                                                        sizeof(AstArithmeticFactor));
            token = travelAst(token, astArithmeticItemMore->arithmeticFactor, NODE_ARITHMETIC_FACTOR);
            if (token->tokenType == TOKEN_OPERATOR) {
                if (strcmp(token->content, "*") == 0
                    || strcmp(token->content, "/") == 0
                    || strcmp(token->content, "%") == 0) {
                    astArithmeticItemMore->arithmeticItemMore = (AstArithmeticItemMore *) pccMalloc(SYNTAX_TAG,
                                                                                                    sizeof(AstArithmeticItemMore));
                    token = travelAst(token, astArithmeticItemMore->arithmeticItemMore,
                                      NODE_ARITHMETIC_ITEM_MORE);
                } else {
                    astArithmeticItemMore->arithmeticItemMore = nullptr;
                }
            } else {
                astArithmeticItemMore->arithmeticItemMore = nullptr;
            }
            break;
        }
        case NODE_ARITHMETIC_FACTOR: {
            AstArithmeticFactor *astArithmeticFactor = (AstArithmeticFactor *) currentNode;
            if (token->tokenType == TOKEN_IDENTIFIER) {
                if (token->next->tokenType == TOKEN_BOUNDARY && strcmp(token->next->content, "(") == 0) {
                    //method return val
                    if (hasMethodDefine(token->content)) {
                        astArithmeticFactor->factorType = ARITHMETIC_METHOD_RET;
                        astArithmeticFactor->methodCall = (AstStatementMethodCall *) pccMalloc(SYNTAX_TAG,
                                                                                               sizeof(AstStatementMethodCall));
                        token = travelAst(token, astArithmeticFactor->methodCall, NODE_STATEMENT_METHOD_CALL);
                        //do not consume method call's ;
                    } else {
                        loge(SYNTAX_TAG, "[-]error: undefined method: %s", token->content);
                        return nullptr;
                    }
                } else {
                    if (hasVarDefine(token->content)) {
                        astArithmeticFactor->factorType = ARITHMETIC_IDENTITY;
                        astArithmeticFactor->identity = (AstIdentity *) pccMalloc(SYNTAX_TAG, sizeof(AstIdentity));
                        astArithmeticFactor->identity->name = token->content;
                        //consume identifier
                        token = token->next;
                    } else {
                        loge(SYNTAX_TAG, "[-]error: undefined var: %s", token->content);
                        return nullptr;
                    }
                }
            } else if (token->tokenType == TOKEN_INTEGER) {
                char *consumedCharsPtr;
                long num = strtol(token->content, &consumedCharsPtr, 10);
                if (consumedCharsPtr == token->content) {
                    loge(SYNTAX_TAG, "[-]error: can not convert int: %s", token->content);
                    return nullptr;
                }
                if (num <= CHAR_MAX) {
                    astArithmeticFactor->factorType = ARITHMETIC_CHAR;
                    astArithmeticFactor->dataChar = (char) num;
                } else if (num <= SHRT_MAX) {
                    astArithmeticFactor->factorType = ARITHMETIC_SHORT;
                    astArithmeticFactor->dataShort = (short) num;
                } else if (num <= INT_MAX) {
                    astArithmeticFactor->factorType = ARITHMETIC_INT;
                    astArithmeticFactor->dataInt = (int) num;
                } else if (num <= LONG_MAX) {
                    astArithmeticFactor->factorType = ARITHMETIC_LONG;
                    astArithmeticFactor->dataLong = (long) num;
                } else {
                    loge(SYNTAX_TAG, "[-]error: int out of range: %s", token->content);
                    return nullptr;
                }
                //consume integer
                token = token->next;
            } else if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, "(")) {
                loge(SYNTAX_TAG, "[-]error: not impl yet: %s", token->content);
                return nullptr;
            } else {
                loge(SYNTAX_TAG, "[-]error: except valid identifier or num: %s", token->content);
                return nullptr;
            }
            break;
        }
        case NODE_STATEMENT_METHOD_CALL: {
            AstStatementMethodCall *astStatementMethodCall = (AstStatementMethodCall *) currentNode;
            astStatementMethodCall->identity = (AstIdentity *) pccMalloc(SYNTAX_TAG, sizeof(AstIdentity));
            astStatementMethodCall->identity->name = token->content;
            token = token->next;
            //fill method call ret type
            AstMethodDefine *methodDefine = getMethodDefine(astStatementMethodCall->identity->name);
            if (methodDefine == nullptr) {
                loge(SYNTAX_TAG, "can not found method define when call method:%s",
                     astStatementMethodCall->identity->name);
                exit(1);
            }
            astStatementMethodCall->retType = methodDefine->type->primitiveType;
            //check next token
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, "(") != 0) {
                loge(SYNTAX_TAG, "[-]error: method call need (: %s", token->content);
            }
            //consume (
            token = token->next;
            if (token->tokenType == TOKEN_BOUNDARY && strcmp(token->content, ")") == 0) {
                astStatementMethodCall->objectList = nullptr;
            } else {
                astStatementMethodCall->objectList = (AstObjectList *) pccMalloc(SYNTAX_TAG,
                                                                                 sizeof(AstObjectList));
                token = travelAst(token, astStatementMethodCall->objectList,
                                  NODE_OBJECT_LIST);
            }
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ")") != 0) {
                loge(SYNTAX_TAG, "[-]error: method call need ): %s", token->content);
            }
            //consume )
            token = token->next;
            if (token->tokenType != TOKEN_BOUNDARY || strcmp(token->content, ";") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ;: %s", token->content);
            }
            //DO NOT consume ;
            break;
        }
        case NODE_EXPRESSION_BOOL: {
            AstExpressionBool *astExpressionBool = (AstExpressionBool *) currentNode;
            astExpressionBool->boolItem = (AstBoolItem *) pccMalloc(SYNTAX_TAG, sizeof(AstBoolItem));
            token = travelAst(token, astExpressionBool->boolItem, NODE_BOOL_ITEM);
            if (token->tokenType == TOKEN_BOOL && strcmp(token->content, "||") == 0) {
                //must be ||
                astExpressionBool->next = (AstExpressionBool *) pccMalloc(SYNTAX_TAG, sizeof(AstExpressionBool));
                token = travelAst(token, astExpressionBool->next, NODE_EXPRESSION_BOOL);
            } else {
                astExpressionBool->next = nullptr;
            }
            break;
        }
        case NODE_BOOL_ITEM: {
            AstBoolItem *astBoolItem = (AstBoolItem *) currentNode;
            astBoolItem->boolFactor = (AstBoolFactor *) pccMalloc(SYNTAX_TAG, sizeof(AstBoolFactor));
            token = travelAst(token, astBoolItem->boolFactor, NODE_BOOL_FACTOR);
            if (token->tokenType == TOKEN_BOOL && strcmp(token->content, "&&") == 0) {
                // must be &&
                astBoolItem->next = (AstBoolItem *) pccMalloc(SYNTAX_TAG, sizeof(AstBoolItem));
                token = travelAst(token, astBoolItem->next, NODE_BOOL_ITEM);
            } else {
                astBoolItem->next = nullptr;
            }
            break;
        }
        case NODE_BOOL_FACTOR: {
            AstBoolFactor *astBoolFactor = (AstBoolFactor *) currentNode;
            if (strcmp(token->content, "!") == 0) {
                astBoolFactor->boolFactorType = BOOL_FACTOR_INVERT;
                astBoolFactor->invertBoolFactor = (AstBoolFactorInvert *) pccMalloc(SYNTAX_TAG,
                                                                                    sizeof(AstBoolFactorInvert));
                token = travelAst(token, astBoolFactor->invertBoolFactor, NODE_BOOL_FACTOR_INVERT);
            } else {
                astBoolFactor->boolFactorType = BOOL_FACTOR_RELATION;
                astBoolFactor->arithmeticBoolFactor = (AstBoolFactorCompareArithmetic *) pccMalloc(SYNTAX_TAG,
                                                                                                   sizeof(AstBoolFactorCompareArithmetic));
                token = travelAst(token, astBoolFactor->arithmeticBoolFactor,
                                  NODE_BOOL_FACTOR_COMPARE_ARITHMETIC);
            }
            break;
        }
        case NODE_BOOL_FACTOR_INVERT: {
            AstBoolFactorInvert *astBoolFactorInvert = (AstBoolFactorInvert *) currentNode;
            if (token->tokenType != TOKEN_OPERATOR || strcmp(token->content, "!") != 0) {
                loge(SYNTAX_TAG, "[-]error: need ;: %s", token->content);
                return nullptr;
            }
            //consume !
            token = token->next;
            astBoolFactorInvert->boolFactor = (AstBoolFactor *) pccMalloc(SYNTAX_TAG, sizeof(AstBoolFactor));
            token = travelAst(token, astBoolFactorInvert->boolFactor, NODE_BOOL_FACTOR);
            break;
        }
        case NODE_BOOL_FACTOR_COMPARE_ARITHMETIC: {
            AstBoolFactorCompareArithmetic *astBoolFactorCompareArithmetic = (AstBoolFactorCompareArithmetic *) currentNode;
            astBoolFactorCompareArithmetic->firstArithmeticExpression = (AstExpressionArithmetic *) pccMalloc(
                    SYNTAX_TAG,
                    sizeof(AstExpressionArithmetic));
            token = travelAst(token, astBoolFactorCompareArithmetic->firstArithmeticExpression,
                              NODE_EXPRESSION_ARITHMETIC);

            if (token->tokenType != TOKEN_OPERATOR && token->tokenType != TOKEN_OPERATOR_2) {
                loge(SYNTAX_TAG, "[-]error: need relation operator: %s", token->content);
                return nullptr;
            }
            if (strcmp(token->content, "==") == 0) {
                astBoolFactorCompareArithmetic->relationOperation = RELATION_EQ;
            } else if (strcmp(token->content, "!=") == 0) {
                astBoolFactorCompareArithmetic->relationOperation = RELATION_NOT_EQ;
            } else if (strcmp(token->content, ">") == 0) {
                astBoolFactorCompareArithmetic->relationOperation = RELATION_GREATER;
            } else if (strcmp(token->content, ">=") == 0) {
                astBoolFactorCompareArithmetic->relationOperation = RELATION_GREATER_EQ;
            } else if (strcmp(token->content, "<") == 0) {
                astBoolFactorCompareArithmetic->relationOperation = RELATION_LESS;
            } else if (strcmp(token->content, "<=") == 0) {
                astBoolFactorCompareArithmetic->relationOperation = RELATION_LESS_EQ;
            } else {
                loge(SYNTAX_TAG, "[-]error: unknown relation operator: %s", token->content);
                return nullptr;
            }
            //consume relation op
            token = token->next;

            astBoolFactorCompareArithmetic->secondArithmeticExpression = (AstExpressionArithmetic *) pccMalloc(
                    SYNTAX_TAG,
                    sizeof(AstExpressionArithmetic));
            token = travelAst(token, astBoolFactorCompareArithmetic->secondArithmeticExpression,
                              NODE_EXPRESSION_ARITHMETIC);
            break;
        }
    }
    return token;
}

AstProgram *buildAst(Token *token) {
    logd(SYNTAX_TAG, "syntax analysis...");
    AstProgram *program = (AstProgram *) pccMalloc(SYNTAX_TAG, sizeof(AstProgram));
    if (token != nullptr && token->tokenType == TOKEN_HEAD) {
        token = token->next;
    }
    travelAst(token, program, NODE_PROGRAM);
    return program;
}

void releaseAstMemory() {
    pccFreeSpace(SYNTAX_TAG);
}