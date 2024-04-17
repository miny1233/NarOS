//
// Created by 谢子南 on 2023/9/7.
//
#include <nar/globa.h>
#include <type.h>
#include "memory.h"
#include "nar/panic.h"

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

static void load_segment()
{
    asm (" movl $0x10,%eax\n"
         " movl %eax,%ds\n"
         " movl %eax,%es\n"
         " movl %eax,%fs\n"
         " movl %eax,%gs\n"
         " movl %eax,%ss\n");
    asm ("ljmp $8,$endofload\n"
         "endofload:\n");
}

void globa_init()
{
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
    desc = gdt + USER_CODE_IDX;
    descriptor_init(desc, 0, 0xFFFFF);
    desc->segment = 1;     // 代码段
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是 64 位
    desc->present = 1;     // 在内存中
    desc->DPL = 3;         // 用户特权级
    desc->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    desc = gdt + USER_DATA_IDX;
    descriptor_init(desc, 0, 0xFFFFF);
    desc->segment = 1;     // 数据段
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 位
    desc->long_mode = 0;   // 不是 64 位
    desc->present = 1;     // 在内存中
    desc->DPL = 3;         // 用户特权级
    desc->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    //设置指针
    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1; // limit是指index的最大位置，而不是gdt的大小
    asm volatile("lgdt gdt_ptr\n");

    //从grub的段切换到内核的段
    load_segment();

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
}