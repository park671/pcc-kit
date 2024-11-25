cmake_minimum_required(VERSION 3.28)
project(pcc VERSION 0.3.001)

configure_file(
        "${CMAKE_SOURCE_DIR}/config.h.in"
        "${CMAKE_BINARY_DIR}/config.h"
)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        ${CMAKE_BINARY_DIR}
        "memory"
        "logger"
        "file"
        "compiler"
        "generator"
        "wrapper"
)

file(GLOB_RECURSE LOGGER_SRC
        "logger/*.cc"
        "logger/*.c"
        "logger/*.cpp"
)

file(GLOB_RECURSE MEMORY_SRC
        "memory/*.cc"
        "memory/*.c"
        "memory/*.cpp"
)

file(GLOB_RECURSE FILE_SRC
        "file/*.cc"
        "file/*.c"
        "file/*.cpp"
)

file(GLOB_RECURSE COMPILER_SRC
        "compiler/*.cc"
        "compiler/*.c"
        "compiler/*.cpp"
)

file(GLOB_RECURSE GENERATOR_SRC
        "generator/*.cc"
        "generator/*.c"
        "generator/*.cpp"

        "generator/arm64/*.cc"
        "generator/arm64/*.c"
        "generator/arm64/*.cpp"
)

file(GLOB_RECURSE WRAPPER_SRC
        "wrapper/*.cc"
        "wrapper/*.c"
        "wrapper/*.cpp"
)

add_executable(pcc
        main.cpp
        ${LOGGER_SRC}
        ${MEMORY_SRC}
        ${FILE_SRC}
        ${COMPILER_SRC}
        ${GENERATOR_SRC}
        ${WRAPPER_SRC}
        generator/arm64/register_arm64.h
        generator/assembler.cpp
        generator/assembler.h
        generator/arm64/register_arm64.cpp
        generator/arm64/linux_syscall.h
        generator/arm64/internal_func_arm64.cpp
        generator/arm64/internal_func_arm64.h
)

add_custom_command(
        TARGET pcc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        COMMENT "Copying include directory to build output"
)
