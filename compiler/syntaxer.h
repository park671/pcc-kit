//
// Created by Park Yu on 2024/9/11.
//

#ifndef PCC_CC_SYNTAXER_H
#define PCC_CC_SYNTAXER_H

#include "ast.h"
#include "token.h"

AstProgram *buildAst(Token *token);

void releaseAstMemory();

#endif //PCC_CC_SYNTAXER_H
