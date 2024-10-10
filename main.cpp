#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include "logger/logger.h"
#include "lexer.h"
#include "syntaxer.h"
#include "mir.h"
#include "optimization.h"
#include "asm_arm64.h"

#define MAIN_TAG "main"

static char *sourceFileName;
static char *outputFileName;


static void usage(int exitcode) {
    fprintf(exitcode ? stderr : stdout,
            "micro c comiler v0.1.102\n"
            "Usage: mcc [-S|p|O|h] <file>\n\n"
            "\n"
            "  -S                Stop before assembly (default)\n"
            "  -o filename       Output to the specified file\n"
            "  -O<number>        Does nothing at this moment\n"
            "  -h                print this help\n");
    exit(exitcode);
}

static void version() {
    printf("micro c compiler\n");
    printf("version 0.1\n");
    printf("build time:2024-09-11\n");
    exit(0);
}

void processParams(int argc, char **argv) {
    for (;;) {
        int opt = getopt(argc, argv, "O:S:o:h:v");
        if (opt == -1)
            break;
        switch (opt) {
            case 'O':
                logd(MAIN_TAG, "[+] optimize level=%s", optarg);
                break;
            case 'S':
                logd(MAIN_TAG, "[+] output assembly");
                break;
            case 'o':
                logd(MAIN_TAG, "[+] output file=%s", optarg);
                outputFileName = optarg;
                break;
            case 'h':
                logd(MAIN_TAG, "[+] show help");
                usage(0);
            case 'v':
                version();
                break;
            default:
                loge(MAIN_TAG, "[-] unknown opt=%d, arg=%s", opt, optarg);
                usage(1);
        }
    }
    if (optind != argc - 1)
        usage(1);
    sourceFileName = argv[optind];
    logd(MAIN_TAG, "[+] input file name=%s\n", sourceFileName);
}

void printTokenStack(Token *tokens) {
    Token *p = tokens;
    while (p != nullptr) {
        if (p->tokenType != TOKEN_HEAD) {
            logd(MAIN_TAG, "token:%s, content:%s", getTokenTypeName(p->tokenType), p->content);
        }
        p = p->next;
    }
}

int main(int argc, char **argv) {
    processParams(argc, argv);
    Token *tokens = buildTokens(sourceFileName);
    AstProgram *program = buildAst(tokens);
    releaseLexerMemory();
    Mir *mir = generateMir(program);
    releaseAstMemory();
    mir = optimize(mir);
    generateArm64Asm(mir, outputFileName);
    return 0;
}
