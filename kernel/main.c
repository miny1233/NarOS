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
#include <nar/fs.h>
#include <nar/multiboot.h>
#include <nar/globa.h>

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

multiboot_info_t* device_info;
int init(unsigned long magic, multiboot_info_t* _info)
{
    tty_init();         // 最早初始化 (printk依赖)

    if (magic != 0x2BADB002)panic("Non-GRUB Boot Kernel\n");
    printk("GRUB Booted Kernel\n");
    device_info = _info;

    globa_init();       // 切换内核描述符表 设置TSS
    interrupt_init();   // 中断处理
    memory_init();      // 内存管理
    task_init();        // 任务调度
    //fs_init();          // 文件系统初始化
    //pipe_init();
    // 外围设备
    interrupt_hardler_register(0x21,keyboard_handler);
    set_interrupt_mask(1,1); //启动键盘中断
    
    //task_create(child);

    if (device_info->flags & 1<<1)
        printk ("boot_device = 0x%x\n", (unsigned) device_info->boot_device);

    
    return 0; //初始化完毕，初始化程序变idle程序
}