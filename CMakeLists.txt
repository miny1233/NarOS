cmake_minimum_required (VERSION 2.8)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set (CMAKE_C_COMPILER "gcc")
set (CMAKE_CXX_COMPILER "g++")
add_compile_options(-m32 -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector)

project (NarOS)

add_executable(main kernel/main.c)
