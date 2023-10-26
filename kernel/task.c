#include <type.h>
#include <nar/printk.h>
#include <device/io.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/mem.h>
#include <memory.h>
#include <nar/panic.h>

#include "nar/debug.h"

task_t *running;        // 当前运行的任务
size_t process_num = 0; // 运行任务数
task_t *task_list;  // 任务列表 所有任务在这里统一管理
pid_t pid_total = 0;

void clock_int(int vector)
{
    assert(vector == 0x20);
    send_eoi(vector);
    task_t *back_task = running;
    next_task:
     running = running->next;
    if(process_num > 1 && running->pid == 0)goto next_task;
    schedule(back_task, running);
}

// 以下是初始化过程
void task_init()
{
    printk("[task]init now!\n");

    task_list = get_page(); //为进程表分配内存
    // 手动加载进程 0
    task_list[0].pid = 0;
    task_list[0].next = &task_list[0];  // 循环链表 自己指向自己
    process_num=1;  // 似乎出现了问题，process默认不为0
    running = &task_list[0];

    // 配置时钟中断
    assert(CLOCK_INT_COUNT_PER_SECOND >= 19);
    u16 hz = (u16)CLOCK_INT_HZ;   // 振荡器的频率大概是 1193182 Hz
    // 控制字寄存器 端口号 0x43
    outb(0x43,0b00110100);  // 计数器 0 先读写低字节后读写高字节 模式2 不使用BCD
    outb(0x40,hz & 0xff);   // 计数器 0 端口号 0x40，用于产生时钟信号 它采用工作方式 3
    outb(0x40,(hz >> 8) & 0xff);

    interrupt_hardler_register(0x20, clock_int);    // 注册中断处理
    set_interrupt_mask(0,1);    // 允许时钟中断

}
// 初始化中断栈
static void stack_init(int_frame* stack,void* entry)
{
    extern u32 interrupt_handler_0x20;  //时钟中断
    stack->ret = (u32)&interrupt_handler_0x20 + 19;  //需要使用取地址符号 外部声明是u32函数会被当作变量
    stack->vector = 0x20;
    stack->ebp = (u32)stack + sizeof(int_frame);
    stack->esp = stack->ebp;
    stack->eip = (u32)entry;
    stack->cs = KERNEL_CODE_SELECTOR;
    stack->eflags=582;
}
// 创建任务 需要提供程序入口
task_t* task_create(void *entry) {
    assert(process_num <= MAX_TASK_NUM); // 任务是否超过最大限度
    asm("cli"); // 保证原子操作 否则可能会调度出错
    // 为新任务设置内存
    void* start_mem = get_page();   //申请一页内存 4k
    //对于end_mem的操作可能有人会问这不内存越界了
    //实际上intel处理器的入栈操作是放入内存[esp - size]
    //而并非先push再减小esp
    //这种设计是好的，只需要esp的地址是对齐的，那么就不可能内存不对齐
    void* end_mem = start_mem + PAGE_SIZE;  // 最后一个地址 + 1
    void* stack_mem = end_mem - sizeof(int_frame);// 内存对齐
    stack_init(stack_mem,entry);
    for(u32 task_idx=1;task_idx < MAX_TASK_NUM;task_idx++)
    {
        if(task_list[task_idx].pid == 0)    //无任务
        {
            // 设置进程信息
            task_list[task_idx].pid = ++pid_total;
            task_list[task_idx].next = running->next;
            task_list[task_idx].esp = (u32)&(((int_frame*)stack_mem)->ret);
            task_list[task_idx].ebp = (u32)stack_mem + sizeof(int_frame);
            running->next = &task_list[task_idx];
            process_num++;  //运行任务数+1
            asm("sti");
            return &task_list[task_idx];
        }
    }
    panic("Have Some Error in Task Create"); // 这个地方理论上不会发生 如果发生那么就是未知错误
}
//还需要有释放内存的功能
void task_exit()
{
    asm("cli\n");
    task_t* back = &task_list[0];// 任务0是常驻任务 从这个地方开始寻找的时间总是比从running开始快得多
    while (back->next != running){
       back = back->next;   // 找到自己的上一个任务
    }
    back->next = running->next;
    running->pid = 0;   //标记任务无效
    process_num--;  // 运行任务数-1
    asm("sti\n");
    yield();   //切换任务
}
