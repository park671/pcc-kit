#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "logger/logger.h"
#include "compiler/lexer.h"
#include "compiler/syntaxer.h"
#include "compiler/mir.h"
#include "compiler/optimization.h"
#include "config.h"
#include "assembler.h"

#define MAIN_TAG "main"

static const char *outputFileName = NULL;
static const char *sourceFileName = NULL;
static Arch targetArch = ARCH_ARM64;
static Platform targetPlatform = PLATFORM_LINUX;
static int optimizationLevel = -1;
static int outputAssembly = 0;
static int sharedLib = 0;
static int fpic = 0;

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
        int opt = getopt(argc, argv, "O:o:a:p:s:f:Shv");
        if (opt == -1)
            break;
        switch (opt) {
            case 'O':
                optimizationLevel = atoi(optarg);
                logd(MAIN_TAG, "[+] optimize level=%d", optimizationLevel);
                break;
            case 'S':
                logd(MAIN_TAG, "[+] output assembly");
                outputAssembly = 1;
                break;
            case 'o':
                logd(MAIN_TAG, "[+] output file=%s", optarg);
                outputFileName = optarg;
                break;
            case 'a':
                logd(MAIN_TAG, "[+] target architecture=%s", optarg);
                targetArch = ARCH_UNKNOWN;
                if (strcmp(optarg, "arm64") == 0) {
                    targetArch = ARCH_ARM64;
                } else if (strcmp(optarg, "x86_64") == 0) {
                    targetArch = ARCH_X86_64;
                }
                break;
            case 'p':
                logd(MAIN_TAG, "[+] target platform=%s", optarg);
                targetPlatform = PLATFORM_UNKNOWN;
                if (strcmp(optarg, "linux") == 0) {
                    targetPlatform = PLATFORM_LINUX;
                } else if (strcmp(optarg, "macos") == 0) {
                    targetPlatform = PLATFORM_MACOS;
                } else if (strcmp(optarg, "windows") == 0) {
                    targetPlatform = PLATFORM_WINDOWS;
                } else if (strcmp(optarg, "bare") == 0) {
                    targetPlatform = PLATFORM_BARE;
                }
                break;
            case 'f':
                if (optarg != nullptr && strcmp("pic", optarg) == 0) {
                    logd(MAIN_TAG, "[+] position independent code (fPIC)");
                    fpic = 1;
                }
                break;
            case 'h':
                logd(MAIN_TAG, "[+] show help");
                usage(0);
                break;
            case 'v':
                version();
                exit(0);
                break;
            case 's':
                if (optarg != nullptr && strcmp("hared", optarg) == 0) {
                    logd(MAIN_TAG, "[+] generate shared library");
                    sharedLib = 1;
                }
                break;
            default:
                loge(MAIN_TAG, "[-] unknown opt=%d, arg=%s", opt, optarg);
                usage(1);
        }
    }
    if (targetArch == ARCH_UNKNOWN) {
        loge(MAIN_TAG, "unknown arch");
        usage(1);
    }

    if (targetPlatform == PLATFORM_UNKNOWN) {
        loge(MAIN_TAG, "unknown platform");
        usage(1);
    }

    // Ensure that the input source file is provided
    if (optind != argc - 1) {
        usage(1);
    }

    sourceFileName = argv[optind];
    logd(MAIN_TAG, "[+] input file name=%s", sourceFileName);

    // Further processing logic (such as handling default behaviors or flags) goes here
}

int main(int argc, char **argv) {
    processParams(argc, argv);
    Token *tokens = buildTokens(sourceFileName);
    printTokenStack(tokens);
    AstProgram *program = buildAst(tokens);
    releaseLexerMemory();
    Mir *mir = generateMir(program);
    releaseAstMemory();
    mir = optimize(mir, optimizationLevel);
    printMir(mir);
    generateTargetFile(mir,
                       targetArch,
                       targetPlatform,
                       outputAssembly,
                       sharedLib,
                       outputFileName);
    return 0;
}
