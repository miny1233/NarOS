#include <type.h>
#include <nar/printk.h>
#include <device/io.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/mem.h>
#include <memory.h>
#include <nar/panic.h>
#include <nar/heap.h>
#include <errno.h>
#include <nar/fs/vfs.h>

task_t *running;        // 当前运行的任务

size_t process_num = 0; // 运行任务数

// 任务0
task_t root_task_pcb;
struct mm_struct root_mm;

pid_t pid_total = 0;

extern tss_t tss;

extern void interrupt_handler_ret_0x20();  //时钟中断返回地址

// 取出根任务
// 内核最基本的任务 通过这个进程来保证内核任务的内存完整性
task_t* get_root_task()
{
    return &root_task_pcb;
}

static pcb_t* get_empty_pcb()
{
    pcb_t* pcb = kalloc(sizeof (pcb_t));

    if(pcb)
        memset(pcb,0,sizeof (pcb_t));

    return pcb;
}

static void clock_int(int vector)
{
    assert(vector == 0x20);
    send_eoi(vector);

    schedule();
}

void schedule()
{
    //寻找下一个任务
    volatile task_t *back_task = running;

    next_task:
    running = running->next;
    if(process_num > 1 && running->pid == 0)
        goto next_task;

    if(back_task == running)
        return;

    struct mm_struct* mm = running->mm;

    // 如果中断涉及DPL改变 会使用ss0 esp0替换
    if(running->dpl != 0)
        tss.esp0 = (u32)mm->kernel_stack_start + PAGE_SIZE;

    // 是否需要切换页目录
    if(get_cr3() != mm->pde)
        set_cr3(mm->pde);

    context_switch(back_task, running);
}

// 以下是初始化过程 禁止使用堆内存
void task_init()
{
    printk("[task]init now!\n");
    // 手动加载进程 0
    root_task_pcb.pid = 0;
    root_task_pcb.next = &root_task_pcb;  // 循环链表 自己指向自己
    root_task_pcb.dpl = 0;

    root_task_pcb.mm = &root_mm;

    // 内存描述符初始化
    init_mm_struct(&root_mm);

    process_num++;
    running = &root_task_pcb;

    // 配置时钟中断
    assert(CLOCK_INT_COUNT_PER_SECOND >= 19);
    u16 hz = (u16)CLOCK_INT_HZ;   // 振荡器的频率大概是 1193182 Hz
    // 控制字寄存器 端口号 0x43
    outb(0x43,0b00110100);  // 计数器 0 先读写低字节后读写高字节 模式2 不使用BCD
    outb(0x40,hz & 0xff);   // 计数器 0 端口号 0x40，用于产生时钟信号 它采用工作方式 3
    outb(0x40,(hz >> 8) & 0xff);

    interrupt_hardler_register(0x20, clock_int);    // 注册中断处理
    set_interrupt_mask(0,1);    // 允许时钟中断

    LOG("load kernel task at pid 0\n");
}

// 初始化ROP中断栈
static void setup_rop_stack(interrupt_stack_frame* rop,void* stack,void* entry,int dpl)
{
    // interrupt_stack_frame* stack = stack_top - sizeof(interrupt_stack_frame);
    rop->ret = (u32)interrupt_handler_ret_0x20;  //需要使用取地址符号 外部声明是u32函数会被当作变量
    rop->vector = 0x20;
    rop->ebp = (u32)(stack + sizeof(interrupt_stack_frame)); // 栈顶
    // rop->esp = (u32)&stack->eip;    //IA32中此值被忽略
    rop->eip = (u32)entry;
    // 加载段选择子
    if (dpl == 0) {
        rop->cs = KERNEL_CODE_SELECTOR;

        rop->gs = KERNEL_DATA_SELECTOR;
        rop->ds = KERNEL_DATA_SELECTOR;
        rop->es = KERNEL_DATA_SELECTOR;
        rop->fs = KERNEL_DATA_SELECTOR;

        rop->eflags = 582;
    } else {
        rop->cs = USER_CODE_SELECTOR;

        rop->gs = USER_DATA_SELECTOR;
        rop->ds = USER_DATA_SELECTOR;
        rop->es = USER_DATA_SELECTOR;
        rop->fs = USER_DATA_SELECTOR;

        rop->ss3 = USER_DATA_SELECTOR;
        rop->esp3 = rop->ebp;

        rop->eflags = (0 << 12 | 0b10 | 1 << 9);
    }
}
// 配置PCB
static void setup_pcb(pcb_t* pcb,const u32* const stack_top,u32 dpl)
{
    pcb->pid = ++pid_total;
    pcb->next = running->next;
    pcb->esp = (u32)stack_top - sizeof(interrupt_stack_frame);
    pcb->ebp = (u32)stack_top;
    pcb->dpl = dpl;
}

static inline void add_pcb_to_queue(pcb_t* pcb)
{
    running->next = pcb;
    process_num++;
}

// 创建内核任务 需要提供程序入口
task_t* task_create(void *entry) {

    if(process_num > MAX_TASK_NUM)
        goto fail;

    set_interrupt_state(0); // 保证原子操作 否则可能会调度出错
    pcb_t* new_task = get_empty_pcb();

    //与主进程共享cr3
    new_task->mm = root_task_pcb.mm;

    // 为新任务设置内存
    void* stack_top = kalloc(PAGE_SIZE) + PAGE_SIZE;  // 栈顶
    interrupt_stack_frame* rop = stack_top - sizeof(interrupt_stack_frame);
    setup_rop_stack(rop,rop,entry,0);
    // 设置进程信息
    setup_pcb(new_task,stack_top,0);
    //下一个任务指向新任务
    add_pcb_to_queue(new_task);

    set_interrupt_state(1); // 启动中断
    return new_task;

fail:
    panic("Have Some Error in Task Create"); // 这个地方理论上不会发生 如果发生那么就是未知错误
    // return 0;
}

// 任务回收
void task_exit()
{
    set_interrupt_state(0);

    /*
     * 从链表中移除当前任务
     * 并删除任务的记录
     */
    task_t* back = get_root_task();// 任务0是常驻任务 从这个地方开始寻找的时间总是比从running开始快得多
    while (back->next != running){
       back = back->next;   // 找到自己的上一个任务
    }
    back->next = running->next;
    running->pid = 0;   //标记任务无效
    process_num--;  // 运行任务数-1

    //释放内存
    if(running->dpl == 3)   // 如果是用户程序还需要释放mm
    {
        free_user_mm_struct(running->mm);
    }

    // 释放PCB
    kfree(running);

    set_interrupt_state(1);
    //任务被移除，需要强制切换来更新
   schedule();
}

// 加载函数到用户空间 并创建用户态进程
pid_t sys_exec(void* function,size_t len)
{
    set_interrupt_state(0); // 保证原子操作 否则可能会调度出错
    // 获取一个PCB
    pcb_t* new_task = get_empty_pcb();

    // 初始化mm
    new_task->mm = kalloc(sizeof (struct mm_struct));
    struct mm_struct* child_mm = new_task->mm;
    init_user_mm_struct(child_mm);

    // 复制代码段
    void* code_segment_begin = sbrk(child_mm,PAGE_SIZE);
    assert(len <= PAGE_SIZE);
    copy_to_mm_space(child_mm,code_segment_begin,function,len);

    // 为用户态程序创建ROP栈 注意需要使用透传
    void* stack_top = sbrk(child_mm,PAGE_SIZE) + PAGE_SIZE;  // 栈顶
    // ROP技术
    interrupt_stack_frame rop_stack;

    // 配置ROP栈
    setup_rop_stack(&rop_stack,stack_top - sizeof(interrupt_stack_frame),code_segment_begin,3);
    // 复制ROP栈
    copy_to_mm_space(child_mm,stack_top - sizeof(interrupt_stack_frame),&rop_stack,sizeof(interrupt_stack_frame));

    // 设置进程信息
    setup_pcb(new_task,stack_top,3);
    add_pcb_to_queue(new_task);

    set_interrupt_state(1); //  任务创建完毕

    return new_task->pid;
}

/*
 *  下面是系统调用定义区
 */

pid_t sys_fork()
{
    return -1;
}

// yield()系统调用
void sys_yield()
{
    schedule();
}

void sys_exit()
{
    task_exit();
}


