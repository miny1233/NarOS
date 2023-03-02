#include<type.h>
#include<nar/printk.h>
#include<device/io.h>
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
}gate_t;

typedef struct pointer
{
    unsigned short limit; // size - 1
    unsigned int base;
} __attribute__((packed)) pointer;

void task_init()
{
    printk("[task]Open Clock\n");
    u16 hz = 1193182/1000;
    outb(0x43,0b00110100);
    outb(0x40,hz&0xff);
    outb(0x42,hz>>8);
}
