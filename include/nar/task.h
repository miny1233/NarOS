#define CLOCK_INT_COUNT_PER_SECOND 19 // 最小值为每秒19次 小于这个值会越界 值无效
#define CLOCK_INT_HZ (1193182 / CLOCK_INT_COUNT_PER_SECOND)  // 最大 65535 否则越界

#define MAX_TASK_NUM 100
#define FD_NR 31

#include <nar/globa.h>
#include <nar/fs/fs.h>
#include "mem.h"

typedef u32 pid_t;

// 双向循环链表
// 本来应该是这样，但是暂时用循环链表实现
// 进程控制块
typedef struct pcb_t
{
    // 任务堆栈状态
    u32 ebp;
    u32 esp;
    // 任务描述
    struct pcb_t* next;    // 下一个任务
    pid_t pid;
    //打开的文件
    struct inode* fd[FD_NR];
    //特权级状态
    u8 dpl; // 特权级
    // 内存
    struct mm_struct* mm;
} pcb_t;

typedef pcb_t task_t;   //task_t 与 pcb_t 都是进程控制块

extern task_t *running; //正在运行的程序

// 中断帧
typedef struct {
    u32 ret;
    u32 vector;
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp;    //这个值在pusha时被保存 但是popa会忽略
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;
    u32 error;
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp3;
    u32 ss3;
}__attribute__((packed)) interrupt_stack_frame;

void context_switch(volatile task_t* this,volatile task_t* next); // 定义在schedule.s中

void schedule();

void task_init();

task_t* task_create(void *entry);

pid_t exec(void* function,size_t len);

void task_exit();

task_t* get_root_task();

#define CLONE_STACK (1L)  // 克隆堆栈
#define CLONE_PTE (1L << 1) // 克隆页表

#define CLONE_FORK (CLONE_STACK | CLONE_PTE) // fork

struct kernel_clone_args
{
    u32 flags;
    u32 stack_size;
};

// 系统调用
pid_t sys_exec(void* function,size_t len);
void sys_yield();
void sys_exit();
pid_t sys_fork();


