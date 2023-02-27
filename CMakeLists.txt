cmake_minimum_required (VERSION 3.8)
include_directories(include) 

enable_language(ASM)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set (CMAKE_ASM_COMPILER "gcc")
set (CMAKE_C_COMPILER "gcc")
set (CMAKE_CXX_COMPILER "g++")
add_compile_options(-Ttext 0 -m32 -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector)

project (NarOS)

aux_source_directory(./kernel KERNEL)
aux_source_directory(./device DEVICE)
add_executable(NarOS ${KERNEL} ${DEVICE})
