//
// Created by Park Yu on 2024/9/11.
//

#ifndef MICRO_CC_SYNTAXER_H
#define MICRO_CC_SYNTAXER_H

#include "ast.h"
#include "token.h"

AstProgram *buildAst(Token *token);

void releaseAst(AstProgram *program);

#endif //MICRO_CC_SYNTAXER_H
