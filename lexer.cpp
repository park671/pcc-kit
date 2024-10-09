//
// Created by Park Yu on 2024/9/11.
//
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "ast.h"
#include "lexer.h"
#include "logger.h"
#include <stack>
#include <string>

using namespace std;

//16KB
#define BUFFER_SIZE 16384

#define LEXER_TAG "lexer"

char *getTokenTypeName(TokenType tokenType) {
    switch (tokenType) {
        case TOKEN_HEAD:
            return "token_head";
        case TOKEN_BOUNDARY:
            return "token_boundary";
        case TOKEN_OPERATOR:
            return "token_operator";
        case TOKEN_OPERATOR_2:
            return "token_operator2";
        case TOKEN_INTEGER:
            return "token_integer";
        case TOKEN_FLOAT:
            return "token_float";
        case TOKEN_KEYWORD:
            return "token_keyword";
        case TOKEN_TYPE:
            return "token_type";
        case TOKEN_IDENTIFIER:
            return "token_identifier";
    }
    return "token_unknown";
}

static void trimString(char *buffer) {
    int startIndex = -1;
    int endIndex = -1;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (startIndex == -1) {
            if (buffer[i] == ' ') {
                continue;
            }
            startIndex = i;
        }
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            endIndex = i;
            break;
        }
    }
    int size = endIndex - startIndex;
    for (int i = 0; i < size; i++) {
        buffer[i] = buffer[i + startIndex];
    }
    buffer[size] = '\0';
}

string keywords[] = {"if", "else", "for", "while", "return"};
string types[] = {"void", "char", "int", "short", "long", "float", "double"};
char boundaries[] = "{}(),;";
char operators[] = "=+-*/<>!";

inline static bool isBoolOperator(char a) {
    if (a == '|' || a == '&') {
        return true;
    }
    return false;
}

inline static bool isOperator(char a) {
    for (int i = 0; i < 8; i++) {
        if (operators[i] == a)return true;
    }
    return false;
}

inline static bool isBoundary(char a) {
    for (int i = 0; i < 6; i++) {
        if (boundaries[i] == a) {
            return true;
        }
    }
    return false;
}

inline static char *makeCharsCopy(const char *origin) {
    int length = strlen(origin) + 1;
    char *copy = (char *) malloc(sizeof(char) * length);
    strcpy(copy, origin);
    return copy;
}

inline static Token *processCompletedString(Token *tail, string &tmp) {
    if (tmp != "") {
        if (tmp[0] >= '0' && tmp[0] <= '9') {
            Token *token = (Token *) (malloc(sizeof(Token)));
            token->tokenType = TOKEN_INTEGER;
            token->content = makeCharsCopy(tmp.c_str());
            tail->next = token;
            tail = token;
//            logd(LEXER_TAG, "[+]integer:%s", tmp.c_str());
        } else {
            bool flag = true;
            for (int i = 0; i < 5; i++) {
                if (keywords[i] == tmp) {
                    Token *token = (Token *) (malloc(sizeof(Token)));
                    token->tokenType = TOKEN_KEYWORD;
                    token->content = makeCharsCopy(tmp.c_str());
                    tail->next = token;
                    tail = token;
//                    logd(LEXER_TAG, "[+]keyword:%s", tmp.c_str());
                    flag = false;
                }
            }
            for (int i = 0; i < 7; i++) {
                if (types[i] == tmp) {
                    Token *token = (Token *) (malloc(sizeof(Token)));
                    token->tokenType = TOKEN_TYPE;
                    token->content = makeCharsCopy(tmp.c_str());
                    tail->next = token;
                    tail = token;
//                    logd(LEXER_TAG, "[+]type:%s", tmp.c_str());
                    flag = false;
                }
            }
            if (flag) {
                Token *token = (Token *) (malloc(sizeof(Token)));
                token->tokenType = TOKEN_IDENTIFIER;
                token->content = makeCharsCopy(tmp.c_str());
                tail->next = token;
                tail = token;
//                logd(LEXER_TAG, "[+]identifier:%s", tmp.c_str());
            }
        }
    }
    tmp = "";
    return tail;
}

static Token *lexer(Token *tail, char *buffer) {
    int length = strlen(buffer);
    string tmp;
    for (int i = 0; i < length; i++) {
        if (buffer[i] == ' ') {
            tail = processCompletedString(tail, tmp);
            continue;
        }
        if (isBoundary(buffer[i])) {
            tail = processCompletedString(tail, tmp);
            Token *token = (Token *) (malloc(sizeof(Token)));
            token->tokenType = TOKEN_BOUNDARY;
            char *content = (char *) malloc(sizeof(char) * 2);
            content[0] = buffer[i];
            content[1] = '\0';
            token->content = content;
            tail->next = token;
            tail = token;
//            logd(LEXER_TAG, "[+]boundary:%c", buffer[i]);
        } else if (isOperator(buffer[i])) {
            tail = processCompletedString(tail, tmp);
            if (i + 1 < length && buffer[i + 1] == '=') {
                Token *token = (Token *) (malloc(sizeof(Token)));
                token->tokenType = TOKEN_OPERATOR_2;
                char *content = (char *) malloc(sizeof(char) * 3);
                content[0] = buffer[i];
                content[1] = buffer[i + 1];
                content[2] = '\0';
                token->content = content;
                tail->next = token;
                tail = token;
//                logd(LEXER_TAG, "[+]operator:%c%c", buffer[i], buffer[i + 1]);
                i++;
            } else {
                Token *token = (Token *) (malloc(sizeof(Token)));
                token->tokenType = TOKEN_OPERATOR;
                char *content = (char *) malloc(sizeof(char) * 2);
                content[0] = buffer[i];
                content[1] = '\0';
                token->content = content;
                tail->next = token;
                tail = token;
//                logd(LEXER_TAG, "[+]operator:%c", buffer[i]);
            }
        } else if (isBoolOperator(buffer[i])) {
            tail = processCompletedString(tail, tmp);
            if (i + 1 < length && buffer[i + 1] == buffer[i]) {
                Token *token = (Token *) (malloc(sizeof(Token)));
                token->tokenType = TOKEN_BOOL;
                char *content = (char *) malloc(sizeof(char) * 3);
                content[0] = buffer[i];
                content[1] = buffer[i + 1];
                content[2] = '\0';
                token->content = content;
                tail->next = token;
                tail = token;
//                logd(LEXER_TAG, "[+]bool:%c%c", buffer[i], buffer[i + 1]);
                i++;
            }
        } else {
            tmp += buffer[i];
        }
    }
    tail = processCompletedString(tail, tmp);
    return tail;
}

Token *buildTokens(const char *sourceFilePath) {
    logd(LEXER_TAG, "lexical analysis...");
    FILE *inputFile = fopen(sourceFilePath, "r");
    char buffer[BUFFER_SIZE];
    Token *tokenHead = (Token *) (malloc(sizeof(Token)));
    tokenHead->tokenType = TOKEN_HEAD;
    Token *tokenTail = tokenHead;
    while (fgets(buffer, BUFFER_SIZE, inputFile) != NULL) {
        trimString(buffer);
//        logd(LEXER_TAG, "%s", buffer);
        tokenTail = lexer(tokenTail, buffer);
    }
    fclose(inputFile);
    return tokenHead;
}