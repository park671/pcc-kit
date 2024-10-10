//
// Created by Park Yu on 2024/9/11.
//

#ifndef MICRO_CC_LEXER_H
#define MICRO_CC_LEXER_H

#include "token.h"

extern Token *buildTokens(const char *sourceFilePath);
extern void releaseLexerMemory();

#endif //MICRO_CC_LEXER_H
