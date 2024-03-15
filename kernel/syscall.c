//
// Created by miny1233 on 2023/2/11.
// 创建这个C的目的就是方便未来添加各种系统调用
//
#include <nar/syscall.h>
#include <nar/printk.h>
//这里声明系统调用函数

//这里定义指针
void* syscall_list[] = {
       
};

void system_call_handle(u32 vector,
                        u32 edi,u32 esi,u32 ebp,u32 esp,
                        u32 ebx,u32 edx,u32 ecx,u32 eax)
{
    printk("syscall called!\n");
    printk("vector is %d\neax is %d\n",vector,eax);
}
