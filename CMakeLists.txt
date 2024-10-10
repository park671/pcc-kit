cmake_minimum_required(VERSION 3.28)
project(pcc VERSION 0.1.103)

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
)

add_executable(pcc
        main.cpp
        ${LOGGER_SRC}
        ${MEMORY_SRC}
        ${FILE_SRC}
        ${COMPILER_SRC}
        ${GENERATOR_SRC}
)
