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
#include <nar/heap.h>
#include <syscall.h>
#include "nar/cpu.h"

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
    // cpu_init();
    memory_init();      // 内存管理
    task_init();        // 进程管理
    heap_init();        // 堆内存管理
    vfs_init();       // 文件系统初始化
    //pipe_init();
    // 设备初始化
    keyboard_init();

    ap_init();  // AP启动

    pid_t pid = exec(child,PAGE_SIZE);
    printk("child pid is %d\n",pid);
    //初始化完毕，初始化程序变idle程序
    return 0;
}

#include <unistd.h>
// 用户态调试函数
void child()
{
    open("/dev/stdout",0);   // sys_open 打开标准输出
    // 打开键盘
    int fd = open("/dev/input",0);     // sys_open 打开键盘
    printf("fd is %d\n",fd);

    int hello = open("/home/hello.text",O_RDONLY);
    printf("hello fd: %d\n",hello);

    if (hello == -1)
    {
        printf("open file fault!\n");
        exit();
    }

    char buf[512];
    memset(buf,0,512);
    read(hello,buf,512);

    printf(buf);
    printf("\n");

    exit();    // sys_exit
}
