//
// Created by 谢子南 on 2024/3/15.
//

// 用户态调用

#ifndef NAROS_SYSCALL_H
#define NAROS_SYSCALL_H

#define _syscall0(nr,ret) \
    asm volatile("int $0x80\n": "=a"(ret): "a"(nr))

#define _syscall1(nr,ret,arg) \
    asm volatile("int $0x80\n": "=a"(ret): "a"(nr), "b"(arg))

#define _syscall2(nr,ret,arg1,arg2) \
    asm volatile("int $0x80\n": "=a"(ret): "a"(nr), "b"(arg1), "c"(arg2))

#define _syscall3(nr,ret,arg1,arg2,arg3) \
    asm volatile("int $0x80\n": "=a"(ret): "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3))

#define _syscall4(nr,ret,arg1,arg2,arg3,arg4) \
    asm volatile("int $0x80\n": "=a"(ret): "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4))

#define _syscall5(nr,ret,arg1,arg2,arg3,arg4,arg5) \
    asm volatile("int $0x80\n": "=a"(ret): "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5))

#endif //NAROS_SYSCALL_H
