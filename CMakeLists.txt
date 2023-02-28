cmake_minimum_required (VERSION 3.8)

project (NarOS)

include_directories(include) 

enable_language(ASM)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set (CMAKE_ASM_COMPILER "gcc")
set (CMAKE_C_COMPILER "gcc")
set (CMAKE_CXX_COMPILER "g++")

set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")


set(CMAKE_EXE_LINKER_FLAGS  "-static -m32 elf_i386 -Ttext 0" )
add_compile_options(-Ttext 0 -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector -m32)

aux_source_directory(./kernel KERNEL)
aux_source_directory(./device DEVICE)

add_executable(NarOS ${KERNEL} ${DEVICE})
