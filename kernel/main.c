#include <type.h>
#include <device/tty.h>
#include <device/keyboard.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <nar/interrupt.h>
#include <nar/syscall.h>
#include <nar/task.h>

int init()
{
  tty_init();
  interrupt_init();
  task_init();  //时钟中断，用于任务调度
  interrupt_hardler_register(0x21,keyboard_handler);
  set_interrupt_mask(1,1); //启动键盘中断
  printk("Welcome to NarOS\n");
  printk("[root@miny1233]#");

  while(1);//主函数返回内核将停机
  return 0; //这样就能通过EAX判断内核是不是正常退出
}

