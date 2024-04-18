#include <nar/interrupt.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <device/io.h>
#include <nar/panic.h>
#include <type.h>
#include <nar/task.h>

#define PIC_M_CTRL 0x20 // 主片的控制端口
#define PIC_M_DATA 0x21 // 主片的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束

// 各种错误信息
static char *messages[] = {
        "#DE Divide Error\0",
        "#DB RESERVED\0",
        "--  NMI Interrupt\0",
        "#BP Breakpoint\0",
        "#OF Overflow\0",
        "#BR BOUND Range Exceeded\0",
        "#UD Invalid Opcode (Undefined Opcode)\0",
        "#NM Device Not Available (No Math Coprocessor)\0",
        "#DF Double Fault\0",
        "    Coprocessor Segment Overrun (reserved)\0",
        "#TS Invalid TSS\0",
        "#NP Segment Not Present\0",
        "#SS Stack-Segment Fault\0",
        "#GP General Protection\0",
        "#PF Page Fault\0",
        "--  (Intel reserved. Do not use.)\0",
        "#MF x87 FPU Floating-Point Error (Math Fault)\0",
        "#AC Alignment Check\0",
        "#MC Machine Check\0",
        "#XF SIMD Floating-Point Exception\0",
        "#VE Virtualization Exception\0",
        "#CP Control Protection Exception\0",
};

// 错误追踪
static void exception_handler(
        int vector,
        u32 edi, u32 esi, u32 ebp, u32 esp,
        u32 ebx, u32 edx, u32 ecx, u32 eax,
        u32 gs, u32 fs, u32 es, u32 ds,u32 error,
        u32 eip, u32 cs, u32 eflags)
{
    char *message = NULL;
    if (vector < 22)
    {
        message = messages[vector];
    }
    else
    {
        message = messages[15];
    }
    printk("\n");
    printk("      PID : %d\n",running->pid);
    printk("EXCEPTION : %s \n", message);
    printk("   VECTOR : 0x%02X\n", vector);
    printk("    ERROR : 0x%08X\n", error);
    printk("   EFLAGS : 0x%08X\n", eflags);
    printk("       CS : 0x%02X\n", cs);
    printk("      EIP : 0x%08X\n", eip);
    printk("      ESP : 0x%08X\n", esp);

    while(1);
    // 通过 EIP 的值应该可以找到出错的位置
    // 也可以在出错时，可以将 hanging 在调试器中手动设置为 0
    // 然后在下面 return 打断点，单步调试，找到出错的位置
}


gate_t idt[IDT_SIZE];
pointer_t idt_48;

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
extern void syscall_handler();//系统调用中断处理

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
    //注册默认的错误的处理
    for(size_t interrupt_num = 0;interrupt_num < IDT_SIZE;interrupt_num++)
        interrupt_hardler_register(interrupt_num,exception_handler);

    // 初始化系统调用
    gate_t *gate = &idt[0x80];
    gate->offset0 = (u32)syscall_handler & 0xffff;
    gate->offset1 = ((u32)syscall_handler >> 16) & 0xffff;
    gate->selector = 1 << 3; // 代码段
    gate->reserved = 0;      // 保留不用
    gate->type = 0b1110;     // 中断门
    gate->segment = 0;       // 系统段
    gate->DPL = 3;           // 用户态
    gate->present = 1;       // 有效

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

// 清除 IF 位，返回设置之前的值
char interrupt_disable()
{
    asm volatile(
            "pushfl\n"        // 将当前 eflags 压入栈中
            "cli\n"           // 清除 IF 位，此时外中断已被屏蔽
            "popl %eax\n"     // 将刚才压入的 eflags 弹出到 eax
            "shrl $9, %eax\n" // 将 eax 右移 9 位，得到 IF 位
            "andl $1, %eax\n" // 只需要 IF 位
            );
}

// 获得 IF 位
char get_interrupt_state()
{
    asm volatile(
            "pushfl\n"        // 将当前 eflags 压入栈中
            "popl %eax\n"     // 将压入的 eflags 弹出到 eax
            "shrl $9, %eax\n" // 将 eax 右移 9 位，得到 IF 位
            "andl $1, %eax\n" // 只需要 IF 位
            );
}

// 设置 IF 位
void set_interrupt_state(char state)
{
    if (state)
        asm volatile("sti\n");
    else
        asm volatile("cli\n");
}
