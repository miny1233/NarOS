#define IDT_SIZE 256
#include <type.h>

void interrupt_init();

void set_interrupt_mask(u32 irq, char enable);
void send_eoi(int vector);// 通知中断控制器，中断处理结束
void interrupt_hardler_register(u32 int_num,void* handler);

char interrupt_disable();             // 清除 IF 位，返回设置之前的值
char get_interrupt_state();           // 获得 IF 位
void set_interrupt_state(char state); // 设置 IF 位