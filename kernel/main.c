#include <type.h>
#include <device/tty.h>
#include <device/keyboard.h>
#include <device/ata.h>
#include <nar/printk.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/pipe.h>
#include <nar/panic.h>
#include <nar/mem.h>
#include <memory.h>
#include <nar/multiboot.h>
#include <nar/globa.h>
#include <nar/fs/vfs.h>

void child();
multiboot_info_t* device_info;

static void check_grub(uint32_t magic)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        panic("Non-GRUB Boot Kernel\n");

    if (device_info->flags & (1 << 9))
        printk("boot by %s\n",device_info->boot_loader_name);
}

int init(unsigned long magic, multiboot_info_t* _info)
{
    device_init();      // 设备初始化
    tty_init();         // 基本显示驱动 (printk依赖)
    device_info = _info;
    check_grub(magic);  // 检查GRUB引导
    globa_init();       // 切换内核描述符表 设置TSS
    interrupt_init();   // 中断处理
    memory_init();      // 内存管理
    task_init();        // 任务调度
    //vfs_init();       // 文件系统初始化
    //pipe_init();
    // 外围设备
    interrupt_hardler_register(0x21,keyboard_handler);
    set_interrupt_mask(1,1); //启动键盘中断

    LOG("start child proc!\n");
    task_create(child);
    LOG("over");
    //create_user_mode_task(child);
    //create_user_mode_task(child);

    int *ptr = NULL;
    ptr = alloc_page(1000);
    printk("stack_start: %x\n",ptr);
    for(int i = 0; i < 300;i++)
    {
        *ptr = 0x1023;
        ptr += 4096;
    }
    printk("stack_start: %x\n",*ptr);

    //初始化完毕，初始化程序变idle程序
    return 0;
}

void child()
{
    while(1);
    int i = 1;
    while(i--)
        printk("%d i am child!\n",i); // 用户态不能使用printk
        while(1);
    task_exit();
}
