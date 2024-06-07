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

void child();
multiboot_info_t* device_info;

static void check_grub(uint32_t magic)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        panic("Non-GRUB Boot Kernel\n");

    if (device_info->flags & (1 << 9))
        printk("boot by %s\n",device_info->boot_loader_name);
}

extern void cpu_init();

int init(unsigned long magic, multiboot_info_t* _info)
{
    device_init();      // 设备初始化
    tty_init();         // 基本显示驱动 (printk依赖)
    device_info = _info;
    check_grub(magic);  // 检查GRUB引导
    globa_init();       // 切换内核描述符表 设置TSS
    interrupt_init();   // 中断处理
    memory_init();      // 内存管理
    task_init();        // 进程管理
    heap_init();        // 堆内存管理
    vfs_init();       // 文件系统初始化
    //pipe_init();
    //cpu_init();
    // 外围设备
    keyboard_init();


    pid_t pid = exec(child,PAGE_SIZE);
    printk("child pid is %d\n",pid);
    //初始化完毕，初始化程序变idle程序
    return 0;
}
// 用户态调试函数
void child()
{
    int fd,invaild;
    _syscall2(1,fd,"/dev/stdout",0);    // sys_open 打开标准输出
    // 打开键盘
    _syscall2(1,fd,"/dev/input",0);     // sys_open 打开键盘

    printf("fd is %d\n",fd);

    for (int count = 0;count < 5;)
    {
        char buf;
        int len;
        _syscall3(2,len,fd,&buf,1); // sys_read 读键盘

        if(len) {
            printf("get input %c\n", buf);
            count++;
        }
        _syscall0(5,invaild);    // sys_yield
    }

    _syscall0(4,invaild);    // sys_exit
}
