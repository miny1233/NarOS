cmake_minimum_required (VERSION 3.8)

project (NarOS)

include_directories(include) 

enable_language(ASM)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

set (CMAKE_C_COMPILER "gcc")
set (CMAKE_CXX_COMPILER "g++")

set(CMAKE_EXE_LINKER_FLAGS  "-Ttext 0 -static -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector -m32" )
add_compile_options(-Ttext 0 -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector -m32)

aux_source_directory(./kernel KERNEL)
aux_source_directory(./device DEVICE)
aux_source_directory(./lib LIB)

add_executable(NarOS ./kernel/head.s 
                     ./kernel/syscall.s
                     ./kernel/interrupt.s 
                     ./device/io.s 
                     ${DEVICE} 
                     ${KERNEL}
                     ${LIB})  
