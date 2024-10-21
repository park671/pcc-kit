//
// Created by Park Yu on 2024/9/11.
//

#ifndef PCC_CC_TOKEN_H
#define PCC_CC_TOKEN_H

enum TokenType {
    TOKEN_HEAD,
    TOKEN_BOUNDARY,
    TOKEN_OPERATOR,
    TOKEN_OPERATOR_2,
    TOKEN_BOOL,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_CHARS,
    TOKEN_KEYWORD,
    TOKEN_TYPE,
    TOKEN_IDENTIFIER
};

struct Token {
    TokenType tokenType;
    const char *content;
    Token *next;
};

#endif //PCC_CC_TOKEN_H
