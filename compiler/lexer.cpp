//
// Created by Park Yu on 2024/9/11.
//
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "logger.h"
#include <stack>
#include "mspace.h"

using namespace std;

//16KB
#define BUFFER_SIZE 16384

static const char *LEXER_TAG = "lexer";
static const char *VAR_TAG = "var";

static const char *keywords[] = {"if", "else", "for", "while", "return"};
static const size_t keywordSize = 5;

static const char *types[] = {"void", "char", "int", "short", "long", "float", "double"};
static const size_t typeSize = 7;

static const char boundaries[] = "{}(),;";
static const size_t boundarySize = 6;

static const char operators[] = "=+-*/<>!";
static const size_t operatorSize = 8;

const char *getTokenTypeName(TokenType tokenType) {
    switch (tokenType) {
        case TOKEN_HEAD:
            return "head";
        case TOKEN_BOUNDARY:
            return "boundary";
        case TOKEN_OPERATOR:
            return "operator";
        case TOKEN_OPERATOR_2:
            return "operator2";
        case TOKEN_INTEGER:
            return "integer";
        case TOKEN_FLOAT:
            return "float";
        case TOKEN_KEYWORD:
            return "keyword";
        case TOKEN_TYPE:
            return "type";
        case TOKEN_IDENTIFIER:
            return "identifier";
        case TOKEN_BOOL:
            return "bool";
        case TOKEN_CHARS:
            return "chars";
        case TOKEN_POINTER:
            return "pointer";
    }
    return "unknown";
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

inline static bool isBoolOperator(char a) {
    if (a == '|' || a == '&') {
        return true;
    }
    return false;
}

inline static bool isOperator(char a) {
    for (int i = 0; i < operatorSize; i++) {
        if (operators[i] == a)return true;
    }
    return false;
}

inline static bool isBoundary(char a) {
    for (int i = 0; i < boundarySize; i++) {
        if (boundaries[i] == a) {
            return true;
        }
    }
    return false;
}

inline static bool isQuotation(char a) {
    return a == '\"';
}

inline static bool isEscape(char a) {
    return a == '\\';
}

inline static bool isPointer(char a) {
    return a == '*';
}

inline static char *makeCharsCopy(const char *origin) {
    int length = strlen(origin) + 1;
    char *copy = (char *) pccMalloc(VAR_TAG, sizeof(char) * length);
    strcpy(copy, origin);
    return copy;
}

inline static bool strEmpty(const char *tmp) {
    if (tmp == nullptr || strlen(tmp) == 0) {
        return true;
    }
    return false;
}

inline static Token *processCompletedString(Token *tail, const char *tmp) {
    if (strEmpty(tmp)) {
        return tail;
    }
    if (tmp[0] >= '0' && tmp[0] <= '9') {
        //integer
        Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
        token->tokenType = TOKEN_INTEGER;
        token->content = makeCharsCopy(tmp);
        tail->next = token;
        tail = token;
        goto finish;
    } else {
        for (int i = 0; i < keywordSize; i++) {
            if (strcmp(keywords[i], tmp) == 0) {
                Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
                token->tokenType = TOKEN_KEYWORD;
                token->content = makeCharsCopy(tmp);
                tail->next = token;
                tail = token;
                goto finish;
            }
        }
        for (int i = 0; i < typeSize; i++) {
            if (strcmp(types[i], tmp) == 0) {
                Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
                token->tokenType = TOKEN_TYPE;
                token->content = makeCharsCopy(tmp);
                tail->next = token;
                tail = token;
                goto finish;
            }
        }
        Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
        token->tokenType = TOKEN_IDENTIFIER;
        token->content = makeCharsCopy(tmp);
        tail->next = token;
        tail = token;
    }
    finish:
    memset(((void *) tmp), 0, 256);
    return tail;
}

static Token *lexer(Token *tail, const char *buffer) {
    size_t length = strlen(buffer);
    char tmp[256];
    memset(((void *) tmp), 0, 256);
    bool nextCharEscape = false;
    bool inCharsSession = false;
    for (int i = 0; i < length; i++) {

        if (inCharsSession && isEscape(buffer[i])) {
            nextCharEscape = true;
            continue;
        } else if (isQuotation(buffer[i]) && !nextCharEscape) {
            if (!inCharsSession) {
                tail = processCompletedString(tail, tmp);
            } else {
                Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
                token->tokenType = TOKEN_CHARS;
                token->content = makeCharsCopy(tmp);
                tail->next = token;
                tail = token;
                memset(((void *) tmp), 0, 256);
            }
            inCharsSession = !inCharsSession;
            continue;
        } else if (inCharsSession) {
            snprintf(tmp, 256, "%s%c", tmp, buffer[i]);
            nextCharEscape = false;
            continue;
        } else if (buffer[i] == ' ') {
            tail = processCompletedString(tail, tmp);
            continue;
        } else if (isPointer(buffer[i])) {
            if (tail->tokenType == TOKEN_TYPE) {
                tail->tokenType = TOKEN_POINTER;
                continue;
            }
        } else if (isBoundary(buffer[i])) {
            tail = processCompletedString(tail, tmp);
            Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
            token->tokenType = TOKEN_BOUNDARY;
            char *content = (char *) pccMalloc(VAR_TAG, sizeof(char) * 2);
            content[0] = buffer[i];
            content[1] = '\0';
            token->content = content;
            tail->next = token;
            tail = token;
        } else if (isOperator(buffer[i])) {
            tail = processCompletedString(tail, tmp);
            if (i + 1 < length && buffer[i + 1] == '=') {
                //double operator
                Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
                token->tokenType = TOKEN_OPERATOR_2;
                char *content = (char *) pccMalloc(VAR_TAG, sizeof(char) * 3);
                content[0] = buffer[i];
                content[1] = buffer[i + 1];
                content[2] = '\0';
                token->content = content;
                tail->next = token;
                tail = token;
                i++;
            } else {
                //single operator
                Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
                token->tokenType = TOKEN_OPERATOR;
                char *content = (char *) pccMalloc(VAR_TAG, sizeof(char) * 2);
                content[0] = buffer[i];
                content[1] = '\0';
                token->content = content;
                tail->next = token;
                tail = token;
            }
        } else if (isBoolOperator(buffer[i])) {
            tail = processCompletedString(tail, tmp);
            if (i + 1 < length && buffer[i + 1] == buffer[i]) {
                Token *token = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
                token->tokenType = TOKEN_BOOL;
                char *content = (char *) pccMalloc(VAR_TAG, sizeof(char) * 3);
                content[0] = buffer[i];
                content[1] = buffer[i + 1];
                content[2] = '\0';
                token->content = content;
                tail->next = token;
                tail = token;
                i++;
            }
        } else {
            snprintf(tmp, 256, "%s%c", tmp, buffer[i]);
        }
    }
    tail = processCompletedString(tail, tmp);
    return tail;
}

Token *buildTokens(const char *sourceFilePath) {
    logd(LEXER_TAG, "lexical analysis...");
    FILE *inputFile = fopen(sourceFilePath, "r");
    char buffer[BUFFER_SIZE];
    Token *tokenHead = (Token *) (pccMalloc(LEXER_TAG, sizeof(Token)));
    tokenHead->tokenType = TOKEN_HEAD;
    Token *tokenTail = tokenHead;
    while (fgets(buffer, BUFFER_SIZE, inputFile) != nullptr) {
        trimString(buffer);
        tokenTail = lexer(tokenTail, buffer);
    }
    fclose(inputFile);
    return tokenHead;
}

void printTokenStack(Token *tokens) {
    Token *p = tokens;
    while (p != nullptr) {
        if (p->tokenType != TOKEN_HEAD) {
            logd(LEXER_TAG, "%s :%s", getTokenTypeName(p->tokenType), p->content);
        }
        p = p->next;
    }
}

void releaseLexerMemory() {
    pccFreeSpace(LEXER_TAG);
}