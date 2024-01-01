#include <type.h>
#include <nar/printk.h>
#include <device/io.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/mem.h>
#include <memory.h>
#include <nar/panic.h>
#include <errno.h>

task_t *running;        // 当前运行的任务
size_t process_num = 0; // 运行任务数
task_t task_list[MAX_TASK_NUM];  // 任务列表 所有任务在这里统一管理 (bitmap太大，10个任务结构体就有1.3M 之后得换种方式储存)
pid_t pid_total = 0;
extern tss_t tss;

extern void interrupt_handler_ret_0x20();  //时钟中断返回地址

// 取出根任务
// 内核最基本的任务 通过这个进程来保证内核任务的内存完整性
task_t* get_root_task()
{
    return &task_list[0];
}


// 初始化中断栈
static void stack_init(void* entry,void* stack_top)
{
    interrupt_stack_frame* stack = stack_top - sizeof(interrupt_stack_frame);
    stack->ret = (u32)interrupt_handler_ret_0x20;  //需要使用取地址符号 外部声明是u32函数会被当作变量
    stack->vector = 0x20;
    stack->ebp = (u32)stack_top;
    stack->esp = (u32)&stack->eip;    //IA32中此值被忽略
    stack->eip = (u32)entry;
    stack->cs = KERNEL_CODE_SELECTOR;
    stack->eflags=582;
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

    struct mm_struct* mm = &running->mm;

    // 如果中断涉及DPL改变 会使用ss0 esp0替换
    if(running->dpl != 0)
        tss.esp0 = (u32)mm->kernel_stack_start + PAGE_SIZE;

    // 是否需要切换页目录
    if(get_cr3() != mm->pte)
        set_cr3(mm->pte);
    // 一般情况下页表是不会改变的 所以无需保存

    context_switch(back_task, running);
}

// 以下是初始化过程
void task_init()
{
    printk("[task]init now!\n");
    // 手动加载进程 0
    task_list[0].pid = 0;
    task_list[0].next = &task_list[0];  // 循环链表 自己指向自己
    task_list[0].dpl = 0;

    // 内存描述符初始化
    task_list[0].mm.pte = get_cr3();   // 页表不会自动保存
    task_list[0].mm.brk = (void*)KERNEL_VMA_START;  // 堆内存初始化
    task_list[0].mm.sbrk = task_list[0].mm.brk;

    process_num++;
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

    LOG("load kernel task at pid 0\n");
}

// 创建内核任务 需要提供程序入口
task_t* task_create(void *entry) {

    if(process_num > MAX_TASK_NUM)
        goto fail;

    set_interrupt_state(0); // 保证原子操作 否则可能会调度出错

    // 为新任务设置内存
    void* stack_top = alloc_page(1) + PAGE_SIZE;  // 栈顶
    stack_init(entry,stack_top);

    u32 task_idx;
    pcb_t* new_task;

    for(task_idx=1;task_idx < MAX_TASK_NUM;task_idx++)
    {
        if(task_list[task_idx].pid == 0)    //无任务
        {
            new_task = &task_list[task_idx];
            goto create_task;
        }
    }
    goto fail;

    create_task:
    // 设置进程信息
    new_task->pid = ++pid_total;
    new_task->next = running->next;
    new_task->esp = (u32)stack_top - sizeof(interrupt_stack_frame);
    new_task->ebp = (u32)stack_top;

    //与主进程共享cr3
    new_task->mm.pte = task_list[0].mm.pte;


    new_task->dpl = 0;    // 内核态
    //new_task->mm.kernel_stack = get_page(); //陷入内核态时堆栈

    //下一个任务指向新任务
    running->next = new_task;
    process_num++;  //运行任务数+1

    set_interrupt_state(1); //  任务创建完毕
    return &task_list[task_idx];


    fail:
    panic("Have Some Error in Task Create"); // 这个地方理论上不会发生 如果发生那么就是未知错误
    return 0;
}

// 任务回收
void task_exit()
{
    set_interrupt_state(0);

    task_t* back = &task_list[0];// 任务0是常驻任务 从这个地方开始寻找的时间总是比从running开始快得多
    while (back->next != running){
       back = back->next;   // 找到自己的上一个任务
    }
    back->next = running->next;
    running->pid = 0;   //标记任务无效
    process_num--;  // 运行任务数-1

    // 释放内核堆栈
    //put_page(running->mm.kernel_stack);


    set_interrupt_state(1);
    //任务被移除，需要强制切换来更新
   schedule();
}

//只允许内核调用一次
pid_t create_user_mode_task(void* entry)
{
    set_interrupt_state(0); // 保证原子操作 否则可能会调度出错
    if(process_num > MAX_TASK_NUM) // 任务是否超过最大限度
        goto fail;
    // 为新任务设置内存
    void* stack_top = alloc_page(1) + PAGE_SIZE;  // 栈顶
    // ROP技术
    interrupt_stack_frame* stack = stack_top - sizeof(interrupt_stack_frame);

    stack->ret = (u32)interrupt_handler_ret_0x20;  //需要使用取地址符号 外部声明是u32函数会被当作变量
    stack->vector = 0x20;
    stack->ebp = (u32)stack_top;
    stack->esp = 0;    //IA32 popa忽略这个值
    stack->eip = (u32)entry;

    //设置段寄存器
    stack->cs = USER_CODE_SELECTOR;
    stack->gs = 0;
    stack->ds = USER_DATA_SELECTOR;
    stack->es = USER_DATA_SELECTOR;
    stack->fs = USER_DATA_SELECTOR;

    stack->ss3 = USER_DATA_SELECTOR;
    stack->esp3 = (u32)stack_top;

    //stack->eflags=582;
    stack->eflags=(0 << 12 | 0b10 | 1 << 9);

    u32 task_idx;
    pcb_t* new_task;
    //寻找空的pcb位置
    for(task_idx = 1;task_idx < MAX_TASK_NUM;task_idx++)
        if(task_list[task_idx].pid == 0) {
            new_task = &task_list[task_idx];
            goto set_process;//无任务
        }

    goto fail;

set_process:
    // 设置进程信息
    new_task->pid = ++pid_total;
    new_task->next = running->next;
    new_task->esp = (u32)stack_top - sizeof(interrupt_stack_frame);
    new_task->ebp = (u32)stack_top;

    // 复制当前任务的页表
    fork_mm_struct(&new_task->mm,&running->mm);
    new_task->dpl = 3;    //用户态

    running->next = new_task;
    process_num++;  //运行任务数+1

    set_interrupt_state(1); //  任务创建完毕

    return task_list[task_idx].pid;

fail:
    return -1;
}

static pcb_t* get_empty_pcb()
{
    int task_idx;
    pcb_t *new_task = NULL;

    for(task_idx=1;task_idx < MAX_TASK_NUM;task_idx++)
    {
        if(task_list[task_idx].pid == 0)    //无任务
        {
            new_task = &task_list[task_idx];
            break;
        }
    }

    return new_task;
}

pid_t kernel_clone(struct kernel_clone_args* args)
{
    pcb_t *child = get_empty_pcb();
    pcb_t *father = running;

    if(child == NULL)
        return -1;
    // 复制父进程信息
    memcpy(child,running,sizeof(pcb_t));

    // 取出内存结构
    struct mm_struct *mm = &child->mm;

    //设置pid
    child->pid = pid_total++;

    if (args->flags & CLONE_STACK)
        return -EINVAL;
    if (args->flags & CLONE_PTE)
    {
        // 拷贝页表 不是共享（写时复制基础）
        fork_mm_struct(&child->mm,&father->mm);
    }

    return child->pid;
}

// fork()系统调用
pid_t sys_fork()
{
    struct kernel_clone_args clone_fork = {
            .flags = CLONE_FORK,
    };

    return kernel_clone(&clone_fork);
}

// yield()系统调用
void sys_yield()
{
    schedule();
}


