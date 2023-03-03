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
  task_init();
  u32 ret = syscall();
  set_interrupt_mask(1,1); //clock int
  set_interrupt_mask(1,1); //keyboard int open
//if(ret==8)printk("called int 0x80");
  printk("Welcom to NarOS\n");
  printk("[root@miny1233]# ");
  
  while(1);
  return 0; //这样就能通过EAX判断内核是不是正常退出
}

