#include<type.h>
#include<nar/printk.h>
#include<device/io.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <memory.h>
#include "nar/debug.h"

void clock_int(int vector)
{
    send_eoi(vector);
    static unsigned int count = 0;
    printk("%d\n",count++);
    int i = 1e8;
    while(i--);
}

descriptor_t gdt[GDT_SIZE]; // 内核全局描述符表
pointer_t gdt_ptr;          // 内核全局描述符表指针
tss_t tss;                  // 任务状态段

static void descriptor_init(descriptor_t *desc, u32 base, u32 limit)
{
    desc->base_low = base & 0xffffff;
    desc->base_high = (base >> 24) & 0xff;
    desc->limit_low = limit & 0xffff;
    desc->limit_high = (limit >> 16) & 0xf;
}

void task_init()
{
    printk("[task]init now!\n");
    //初始化全局描述符表
    memset(gdt, 0, sizeof(gdt)); //NULL描述符在此时被初始化

    descriptor_t *desc;
    //初始化内核态
    desc = gdt + KERNEL_CODE_IDX;
    descriptor_init(desc, 0, 0xFFFFF);
    desc->segment = 1;     // 代码段
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是 64 位 (这个段实际上在IA32中是保留位置，只需置0即可)
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 内核特权级
    desc->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    desc = gdt + KERNEL_DATA_IDX;
    descriptor_init(desc, 0, 0xFFFFF);
    desc->segment = 1;     // 数据段
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是 64 位
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 内核特权级
    desc->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    //初始化用户态

    //设置指针
    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1; // limit是指index的最大位置，而不是gdt的大小
    asm volatile("lgdt gdt_ptr\n");

    //内核任务描述符
    memset(&tss, 0, sizeof(tss));

    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iobase = sizeof(tss);

    desc = gdt + KERNEL_TSS_IDX;
    descriptor_init(desc, (u32)&tss, sizeof(tss) - 1);
    desc->segment = 0;     // 系统段
    desc->granularity = 0; // 字节
    desc->big = 0;         // 固定为 0
    desc->long_mode = 0;   // 固定为 0
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 用于任务门或调用门
    desc->type = 0b1001;   // 32 位可用 tss

    asm volatile(
            "ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));

    // 配置时钟中断 (BUG:无法调节中断频率)
    u16 hz = (u16)CLOCK_INT_HZ;   // 振荡器的频率大概是 1193182 Hz
    // 控制字寄存器 端口号 0x43
    outb(0x43,0b00110100);  // 计数器 0 先读写低字节后读写高字节 模式2 不使用BCD
    outb(0x40,hz & 0xff);   // 计数器 0 端口号 0x40，用于产生时钟信号 它采用工作方式 3
    outb(0x40,(hz >> 8) & 0xff);

    interrupt_hardler_register(0x20, clock_int);    // 注册中断处理
    set_interrupt_mask(0,1);    // 允许时钟中断

}
