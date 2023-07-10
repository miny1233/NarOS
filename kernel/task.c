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
size_t process_num = 0; // 累计任务总数 只增不减 也就是说当创建过128个任务就不能创建了 之后会改
task_t task_list[MAX_TASK_NUM];  // 任务列表 所有任务在这里统一管理
pid_t pid_total = 0;

void extend(void)
{
   
}

// 这里面的东西与int_stack有关，修改必须注意
void clock_int(int vector)
{
    send_eoi(vector);
    extend();   // 扩展功能写在这里面，就不会影响调度
    schedule(running,running->next);
    running = running->next;
}

// 以下是初始化过程
descriptor_t gdt[GDT_SIZE]; // 内核全局描述符表
pointer_t gdt_ptr;          // 内核全局描述符表指针
tss_t tss;                  // 任务状态段

static void descriptor_init(descriptor_t *desc, u32 base, u32 limit)
{
    desc->base_low = base & 0xffffff;
    desc->base_high = (base >> 24) & 0xff;
    desc->limit_low = limit & 0xffff;
    desc->limit_high = (limit >> 16) & 0xf;
}

void task_init()
{
    printk("[task]init now!\n");
    //初始化全局描述符表
    memset(gdt, 0, sizeof(gdt)); //NULL描述符在此时被初始化

    descriptor_t *desc;
    //初始化内核态
    desc = gdt + KERNEL_CODE_IDX;
    descriptor_init(desc, 0, 0xFFFFF);
    desc->segment = 1;     // 代码段
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是 64 位 (这个段实际上在IA32中是保留位置，只需置0即可)
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 内核特权级
    desc->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    desc = gdt + KERNEL_DATA_IDX;
    descriptor_init(desc, 0, 0xFFFFF);
    desc->segment = 1;     // 数据段
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是 64 位
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 内核特权级
    desc->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    //初始化用户态

    //设置指针
    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1; // limit是指index的最大位置，而不是gdt的大小
    asm volatile("lgdt gdt_ptr\n");

    //内核任务描述符
    memset(&tss, 0, sizeof(tss));

    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iobase = sizeof(tss);

    desc = gdt + KERNEL_TSS_IDX;
    descriptor_init(desc, (u32)&tss, sizeof(tss) - 1);
    desc->segment = 0;     // 系统段
    desc->granularity = 0; // 字节
    desc->big = 0;         // 固定为 0
    desc->long_mode = 0;   // 固定为 0
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 用于任务门或调用门
    desc->type = 0b1001;   // 32 位可用 tss

    asm volatile(
            "ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));

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
static void stack_init(int_stack* stack,void* entry)
{
    extern u32 interrupt_handler_0x20;  //时钟中断
    // 函数调用产生的栈
    stack->eip2 = (u32)clock_int + 43;  //call schedule的下一条语句
    stack->ebp1 = (u32)stack + sizeof(int_stack);
    stack->eip1 = (u32)&interrupt_handler_0x20 + 19;  //需要使用取地址符号 外部声明是u32函数会被当作变量
    // 中断处理函数创建的栈
    stack->vector = 0x20;   // 中断号 0x20
    stack->ebp = (u32)stack + sizeof(int_stack);
    stack->esp = stack->ebp;
    // Intel中断栈
    stack->eip = (u32)entry; //eip 指向程序入口，中断返回则直接运行新任务
    stack->cs = KERNEL_CODE_SELECTOR;
    stack->eflags = 582; // 默认标记
}
// 创建任务 需要提供程序入口
task_t* task_create(void *entry) {
    assert(process_num <= MAX_TASK_NUM); // 任务是否超过最大限度
    asm("cli"); // 保证原子操作 否则可能会调度出错
    // 为新任务设置内存
    void* start_mem = get_page();   //申请一页内存 4k
    void* end_mem = start_mem + PAGE_SIZE - 1;  // 页尾
    stack_init(end_mem - sizeof(int_stack),entry);
    for(u32 task_idx=1;task_idx < MAX_TASK_NUM;task_idx++)
    {
        if(task_list[task_idx].pid == 0)    //无任务
        {
            // 设置进程信息
            task_list[task_idx].pid = ++pid_total;
            task_list[task_idx].next = running->next;
            task_list[task_idx].esp = (u32)end_mem - sizeof(int_stack);
            task_list[task_idx].ebp = task_list[task_idx].esp + 0x14; // 这个位置指向ebp1
            running->next = &task_list[task_idx];
            process_num++;
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
    asm("sti\n");
    yield();   //切换任务
}
