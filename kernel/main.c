#include <type.h>
#include <device/tty.h>
#include <device/keyboard.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/pipe.h>

int init()
{
  tty_init();  //最早初始化 printk依赖
  pipe_init();
  interrupt_init();
  //task_init();  //任务调度程序
  interrupt_hardler_register(0x21,keyboard_handler);
  set_interrupt_mask(1,1); //启动键盘中断
  return 0; //初始化完毕，初始化程序变idle程序
}

