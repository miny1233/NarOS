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
    u8* buffer = get_page();
    u8 hel[512] = {"wen ben"};
    disk_write(0x40,hel,1);
    disk_read(0x40,buffer,1);
    for(int i = 0;i < 512;i++)
    {
        //printk("%d :",i);
        printk("%c",buffer[i]);
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
    //printk("kernel message:%s : %d","nothing to do",20);

    return 0; //初始化完毕，初始化程序变idle程序
}