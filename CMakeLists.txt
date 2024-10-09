cmake_minimum_required(VERSION 3.28)
project(micro_cc)

set(CMAKE_CXX_STANDARD 17)

add_executable(micro_cc main.cpp
        ast.h
        lexer.cpp
        lexer.h
        logger.cpp
        logger.h
        syntaxer.cpp
        syntaxer.h
        mir.cpp
        mir.h
        token.h
        asm_arm64.cpp
        asm_arm64.h
        bin_arm64.cpp
        bin_arm64.h
        optimization.cpp
        optimization.h
#        test.cpp
)
