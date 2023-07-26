#include <type.h>
#include <device/tty.h>
#include <device/keyboard.h>
#include <device/disk.h>
#include <nar/printk.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/pipe.h>
#include <nar/panic.h>
#include <nar/mem.h>
#include <memory.h>

void child()
{
    void* buffer = get_page();
    for(int sector=0;sector<50;sector++) {
        disk_read(sector, buffer, 10);
        for (int i = 0; i < 5120; i++) {
            int j = 1e4;
            while (--j);
            printk("%c", *((char *)buffer + i));
        }
    }
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

