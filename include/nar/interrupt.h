#define IDT_SIZE 256
#include <type.h>

void interrupt_init();

void set_interrupt_mask(u32 irq, char enable);
void send_eoi(int vector);// 通知中断控制器，中断处理结束