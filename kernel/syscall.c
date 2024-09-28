//
// Created by miny1233 on 2023/2/11.
// 创建这个C的目的就是方便未来添加各种系统调用
//
#include <nar/printk.h>
#include <nar/interrupt.h>
#include <nar/panic.h>
#include <nar/fs/vfs.h>
#include <nar/task.h>

// 测试用系统调用
void sys_log(const char*);
//这里定义指针
void* syscall_list[] = {
        sys_log,
        sys_open,
        sys_read,
        sys_write,
        sys_exit,
        sys_yield,
        sys_exec,
        sys_sbrk,
};

__attribute__((unused))
void system_call_handle(int vector,
                        u32 edi,u32 esi,u32 ebp,u32 esp,
                        u32 ebx,u32 edx,u32 ecx,u32 eax)
{
    assert(vector == 0x80);

    send_eoi(vector);

    //LOG("syscall called!\n");

    int* ret = (int *)&eax;

    //  系统调用无效
    if(eax >= sizeof (syscall_list) / sizeof (void*))
    {
        *ret = -1;
        return;
    }

    int(*handle)(u32,u32,u32,u32,u32) = syscall_list[eax];

    *ret = handle(ebx,ecx,edx,esi,edi);
}

void sys_log(const char* str)
{
    printk(str);
}
