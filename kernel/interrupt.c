#include <nar/interrupt.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <type.h>

typedef struct gate_t
{
    u16 offset0;    // 段内偏移 0 ~ 15 位
    u16 selector;   // 代码段选择子
    u8 reserved;    // 保留不用
    u8 type : 4;    // 任务门/中断门/陷阱门
    u8 segment : 1; // segment = 0 表示系统段
    u8 DPL : 2;     // 使用 int 指令访问的最低权限
    u8 present : 1; // 是否有效
    u16 offset1;    // 段内偏移 16 ~ 31 位
}__attribute__((packed)) gate_t;

typedef struct pointer_t
{
    unsigned short limit; // size - 1
    unsigned int base;
}__attribute__((packed)) pointer_t;


gate_t idt[IDT_SIZE];
pointer_t idt_48;

void int_test(); 

void now_jmp()
{
    printk("get interrupt\n");
}

void interrupt_init()
{
    gate_t idt_s;
    idt_s.offset0 = (u32)int_test & 0xffff;
    idt_s.offset1 = ((u32)int_test>>16) & 0xffff;
    idt_s.selector = 0x8; //Kernel Segment
    idt_s.reserved = 0;//keep zero
    idt_s.type = 0b1110;//int gate
    idt_s.segment = 0;//System Segment
    idt_s.DPL = 0;//kernel
    idt_s.present = 1;//avaliable
    for(size_t i=0;i<IDT_SIZE;i++)
    {
        idt[i] = idt_s;
    }
    idt_48.base = (u32)idt;
    idt_48.limit = sizeof(idt)-1; 
    asm volatile("lidt idt_48");
    asm volatile("sti");//open int
    return;
}