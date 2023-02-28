//
// Created by miny1233 on 2023/2/11.
// 创建这个C的目的就是方便未来添加各种系统调用
//
#include <nar/syscall.h>
#include <nar/pipe.h>

typedef void *syscall_entry; //暂时先做只支持一个参数的调用

//这里声明系统调用函数

//这里定义指针
syscall_entry syscall_List[] = {
        mount,
        unmount,
        write,
        read,
};
