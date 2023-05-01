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
  tty_init();
  pipe_init();
  interrupt_init();
  //task_init();  //时钟中断，用于任务调度
  interrupt_hardler_register(0x21,keyboard_handler);
  set_interrupt_mask(1,1); //启动键盘中断

  //管道功能测试
  char buf[100] = {0},buf2[100];
  pipe_t p1 = pipe_create(buf);
  pipe_t p2 = pipe_create(buf2);
    pipe_wirte(p1,"hello p1\n",11);
    pipe_destory(p2);
    pipe_wirte(p2,"hello p2\n",11);
    printk(buf);
    printk(buf2);

  return 0; //这样就能通过EAX判断内核是不是正常退出
}

