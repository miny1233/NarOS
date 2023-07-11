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
    void* ptr[10];
    for(int i=0;i<10;i++) {
        ptr[i] = get_page();
        printk("got page 0x%x\n",ptr[i]);
    }
    for(int i=0;i<3;i++) {
        put_page(ptr[i]);
        printk("freed page 0x%x\n",ptr[i]);
    }
    for(int i=0;i<10;i++) {
        ptr[i] = get_page();
        printk("got page 0x%x\n",ptr[i]);
    }
    static int count = 1e10;
    while(--count);
    printk("exit");
    task_exit();
}

int init()
{
    tty_init();         // 最早初始化
    interrupt_init();   // 中断处理
    memory_init();      // 内存管理
    task_init();        // 任务调度
    pipe_init();
    // 外围设备
    interrupt_hardler_register(0x21,keyboard_handler);
    set_interrupt_mask(1,1); //启动键盘中断

    task_create(child);

    return 0; //初始化完毕，初始化程序变idle程序
}

