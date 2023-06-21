#include <type.h>
#include <device/tty.h>
#include <device/keyboard.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/pipe.h>
#include <nar/panic.h>
#include <nar/mem.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>

void child()
{
    while(1)
        printk("A");
}
void child2()
{
    while(1)
        printk("B");
}


int init()
{
    tty_init();         // 最早初始化 printk依赖
    interrupt_init();   // 中断处理
    memory_init();      // 内存管理
    task_init();        // 任务调度
    pipe_init();

    // 外围设备
    interrupt_hardler_register(0x21,keyboard_handler);
    set_interrupt_mask(1,1); //启动键盘中断

    task_create(child); //创建任务
    task_create(child2);

    return 0; //初始化完毕，初始化程序变idle程序
}

