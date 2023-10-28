#define CLOCK_INT_COUNT_PER_SECOND 19 // 最小值为每秒19次 小于这个值会越界 值无效
#define CLOCK_INT_HZ (1193182 / CLOCK_INT_COUNT_PER_SECOND)  // 最大 65535 否则越界

#define MAX_TASK_NUM 128

#include <nar/globa.h>
#include <nar/vfs/inode.h>

typedef u32 pid_t;

// 双向循环链表
// 本来应该是这样，但是暂时用循环链表实现
// 进程控制块
typedef struct pcb_t
{
    u32 ebp;
    u32 esp;
    struct pcb_t* next;    // 下一个任务
    pid_t pid;
    //需要用内存位图来管理使用的内存
    //打开的文件
    inode_t files[32];
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
    u32 esp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 eip;
    u32 cs;
    u32 eflags;
}__attribute__((packed)) interrupt_stack_frame;

void schedule(task_t* this,task_t* next); // 定义在schedule.s中

void task_init();

task_t* task_create(void *entry);

void task_exit();

#define yield() asm("int $0x20\n")


