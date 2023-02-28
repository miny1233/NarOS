#include"../include/type.h"
//系统调用(调用方)
void syscall();

//想着用R10寄存器传递数据，但是汇编怎么都过不了，先暂时搁着了
#define _syscall_1(syscall_num,type1,arg1) \
asm volatile(""::"d"(syscall_num)); \
((int(*)(type1))syscall)(arg1)
#define _syscall_2(syscall_num,type1,type2,arg1,arg2) ((int(*)(type1,type2))(syscall))(arg1,arg2)