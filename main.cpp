#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logger/logger.h"
#include "compiler/lexer.h"
#include "compiler/syntaxer.h"
#include "compiler/mir.h"
#include "compiler/optimization.h"
#include "generator/asm_arm64.h"
#include "config.h"

#define MAIN_TAG "main"

static char *sourceFileName;
static char *outputFileName;

static void version() {
    printf("\n");
    printf("                        __         \n"
           "                       /\\ \\        \n"
           " _____      __     _ __\\ \\ \\/'\\    \n"
           "/\\ '__`\\  /'__`\\  /\\`'__\\ \\ , <    \n"
           "\\ \\ \\L\\ \\/\\ \\L\\.\\_\\ \\ \\/ \\ \\ \\\\`\\  \n"
           " \\ \\ ,__/\\ \\__/.\\_\\\\ \\_\\  \\ \\_\\ \\_\\\n"
           "  \\ \\ \\/  \\/__/\\/_/ \\/_/   \\/_/\\/_/\n"
           "   \\ \\_\\                           \n"
           "    \\/_/                           \n");
    printf("park's c compiler kit\n");
    printf("version %s\n", PROJECT_VERSION);
    printf("build time:%s %s\n", __DATE__, __TIME__);
}

static void usage(int exitcode) {
    version();
    fprintf(exitcode ? stderr : stdout,
            "Usage: pcc -[options] <file>\n\n"
            "  -o <filename>        \toutput file path\n"
            "  -O<number>           \toptimization level\n"
            "  -S                   \tcompile to assembly (no binary)\n"
            "  -a <target-arch>     \ttarget cpu inst (arm64, x86_64)\n"
            "  -p <target-platform> \ttarget os platform (linux, macos, windows, bare)\n"
            "  -shared              \twrapper as shared lib\n"
            "  -fpic                \tposition independent code\n"
            "  -h                   \tprint this help\n"
            "\n"
    );
    exit(exitcode);
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
                exit(0);
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
