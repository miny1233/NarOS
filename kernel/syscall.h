#include"type.h"
//系统调用
typedef void*(*syscall_entry)(void*) //暂时先做只支持一个参数的调用

//这里声明系统调用函数
void* fork(void*);
//这里定义指针
syscall_entry _syscall_fork;

syscall_entry syscall_List[] = {
   NULL, //空调用
   _syscall_fork,
};
//已经是内核态了
void* syscall(u32 syscall_number,void* context)
{
    if(sizeof(syscall_List)/sizeof(syscall_entry)<=syscall_number&&syscall_List[syscall_number])return NULL;//错误的调用
    //执行调用
    return syscall_List[syscall_number](context);
}