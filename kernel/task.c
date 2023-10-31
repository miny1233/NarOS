#include <type.h>
#include <nar/printk.h>
#include <device/io.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/mem.h>
#include <memory.h>
#include <nar/panic.h>

volatile task_t *running;        // 当前运行的任务
size_t process_num = 0; // 运行任务数
task_t *task_list;  // 任务列表 所有任务在这里统一管理
pid_t pid_total = 0;

extern void interrupt_handler_ret_0x20();  //时钟中断返回地址

static void clock_int(int vector)
{
    assert(vector == 0x20);
    send_eoi(vector);

    volatile task_t *back_task = running;
    next_task:
        running = running->next;
    if(process_num > 1 && running->pid == 0)goto next_task;
    //if(back_task->pid == running->pid)return;
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
    process_num++;  // 似乎出现了问题，process默认不为0
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

// 创建内核任务 需要提供程序入口
task_t* task_create(void *entry) {
    assert(process_num <= MAX_TASK_NUM); // 任务是否超过最大限度
    set_interrupt_state(0); // 保证原子操作 否则可能会调度出错
    // 为新任务设置内存
    void* stack_top = get_page() + PAGE_SIZE;  // 栈顶
    stack_init(entry,stack_top);
    for(u32 task_idx=1;task_idx < MAX_TASK_NUM;task_idx++)
    {
        if(task_list[task_idx].pid == 0)    //无任务
        {
            // 设置进程信息
            task_list[task_idx].pid = ++pid_total;
            task_list[task_idx].next = running->next;
            task_list[task_idx].esp = (u32)stack_top - sizeof(interrupt_stack_frame);
            task_list[task_idx].ebp = (u32)stack_top;
            task_list[task_idx].cr3 = get_cr3();    //与主进程共享cr3
            running->next = &task_list[task_idx];
            process_num++;  //运行任务数+1

            set_interrupt_state(1); //  任务创建完毕
            return &task_list[task_idx];
        }
    }
    panic("Have Some Error in Task Create"); // 这个地方理论上不会发生 如果发生那么就是未知错误
    return 0;
}

//还需要有释放内存的功能
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

    set_interrupt_state(1);
    //任务被移除，需要强制切换来更新
    yield();
}

pid_t create_user_mode_task(void* entry)
{
    set_interrupt_state(0); // 保证原子操作 否则可能会调度出错
    assert(process_num <= MAX_TASK_NUM); // 任务是否超过最大限度
    // 为新任务设置内存
    void* stack_top = get_page() + PAGE_SIZE;  // 栈顶
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
    //寻找空的pcb位置
    for(task_idx = 1;task_idx < MAX_TASK_NUM;task_idx++)
        if(task_list[task_idx].pid == 0)break;    //无任务

    // 设置进程信息
    task_list[task_idx].pid = ++pid_total;
    task_list[task_idx].next = running->next;
    task_list[task_idx].esp = (u32)stack_top - sizeof(interrupt_stack_frame);
    task_list[task_idx].ebp = (u32)stack_top;
    //现在是测试，应该复制页表
    //task_list[task_idx].cr3 = get_cr3();

    running->next = &task_list[task_idx];
    process_num++;  //运行任务数+1

    set_interrupt_state(1); //  任务创建完毕
    return task_list[task_idx].pid;
}


// fork()系统调用
void sys_fork()
{

}

