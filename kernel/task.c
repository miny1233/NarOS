#include<type.h>
#include<nar/printk.h>
#include<device/io.h>
#include <nar/interrupt.h>
typedef struct
{
    u16 offset0;    // 段内偏移 0 ~ 15 位
    u16 selector;   // 代码段选择子
    u8 reserved;    // 保留不用
    u8 type : 4;    // 任务门/中断门/陷阱门
    u8 segment : 1; // segment = 0 表示系统段
    u8 DPL : 2;     // 使用 int 指令访问的最低权限
    u8 present : 1; // 是否有效
    u16 offset1;    // 段内偏移 16 ~ 31 位
}gate_t; // 门描述符

typedef struct pointer
{
    unsigned short limit; // size - 1
    unsigned int base;
} __attribute__((packed)) pointer; // 这里必须要要告诉编译器不能优化

void clock_int(int vector)
{
    send_eoi(vector);
    printk("%x \n",vector);
}

void task_init()
{
    printk("[task]Open Clock\n");
    u16 hz = 1193182/1000; // 每1ms发出一次中断，这里主要考虑到发出的原始频率是1193182Hz，那么没1193182/1000次所耗时就是1ms
    outb(0x43,0b00110100); // 固定格式
    outb(0x40,hz);         
    outb(0x40,hz>>8);

    set_interrupt_mask(0,1); //时钟中断
    interrupt_hardler_register(0x20, clock_int); //注册中断处理
}
