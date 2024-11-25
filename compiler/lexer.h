//
// Created by Park Yu on 2024/9/11.
//

#ifndef PCC_CC_LEXER_H
#define PCC_CC_LEXER_H

#include "token.h"
#include "preprocessor.h"

extern Token *buildTokens(ProcessedSource *source);

extern void printTokenStack(Token *tokens);

extern void releaseLexerMemory();

#endif //PCC_CC_LEXER_H
