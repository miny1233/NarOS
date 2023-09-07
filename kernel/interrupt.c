#include <nar/interrupt.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <device/io.h>
#include <nar/panic.h>
#include <type.h>

#define PIC_M_CTRL 0x20 // 主片的控制端口
#define PIC_M_DATA 0x21 // 主片的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束

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
volatile pointer_t idt_48;

static void pic_init()
{
    outb(PIC_M_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(PIC_M_DATA, 0x20);       // ICW2: 起始中断向量号 0x20
    outb(PIC_M_DATA, 0b00000100); // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outb(PIC_S_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(PIC_S_DATA, 0x28);       // ICW2: 起始中断向量号 0x28
    outb(PIC_S_DATA, 2);          // ICW3: 设置从片连接到主片的 IR2 引脚
    outb(PIC_S_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outb(PIC_M_DATA, 0b11111111); // 关闭所有中断
    outb(PIC_S_DATA, 0b11111111); // 关闭所有中断
}

extern void interrupt_process(); 
extern void default_int_hardler();

extern u32 handler_entry_table[];

void interrupt_init()
{
    printk("[interrupt]Init PIC\n");
    pic_init();
    printk("[interrupt]Set IDT NOW\n");

    gate_t idt_s;
    idt_s.selector = 0x8;   //系统段
    idt_s.reserved = 0;     //保留0
    idt_s.type = 0b1110;    //中断门
    idt_s.segment = 0;      //系统段
    idt_s.DPL = 0;          //内核态
    idt_s.present = 1;      //有效
    for(size_t interrupt_num=0;interrupt_num<=0x2f;interrupt_num++)
    {
        idt_s.offset0 = (u32)handler_entry_table[interrupt_num];
        idt_s.offset1 = (u32)handler_entry_table[interrupt_num]>>16;
        idt[interrupt_num] = idt_s;
    }
    for(size_t interrupt_num=0x30;interrupt_num<IDT_SIZE;interrupt_num++)
    {
        idt_s.offset0 = (u32)default_int_hardler;
        idt_s.offset1 = (u32)default_int_hardler>>16;
        idt[interrupt_num] = idt_s;
    }


    idt_48.base = (u32)idt;
    idt_48.limit = sizeof(idt)-1; 
    asm volatile("lidt idt_48");
    asm volatile("sti");
}

u32 interrupt_hardler_list[IDT_SIZE];

void interrupt_hardler_register(u32 int_num,void* handler)
{
    interrupt_hardler_list[int_num] = (u32)handler;
}

// 通知中断控制器中断结束
// 进入中断处理后就立即发送 如果当前中断未处理结束下一次中断信号会被CPU阻塞
// 这种提前通知的方式能够省下中断控制器的处理时间 使中断更加高效
void send_eoi(int vector)
{
    if (vector >= 0x20 && vector < 0x28)
    {
        outb(PIC_M_CTRL, PIC_EOI);
    }
    if (vector >= 0x28 && vector < 0x30)
    {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

void set_interrupt_mask(u32 irq, char enable)
{
    u16 port;
    if (irq < 8)
    {
        port = PIC_M_DATA;
    }
    else
    {
        port = PIC_S_DATA;
        irq -= 8;
    }

    if (enable)
        outb(port, inb(port) & ~(1 << irq));
    else
        outb(port, inb(port) | (1 << irq));
}

void interrupt_debug(u32 vector)
{
    if(vector==0)
    {
        printk("Unknown Int Called\n");
        return;
    }
    send_eoi(vector);
    panic("Unknown Error");
}