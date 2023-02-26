#include"printk.c"
#include "../include/string.h"

//没有底层操作系统，初始化变量是无效的
int offset;
char tmp[128];

void outb(u16 des,u8 value);
u8 inb(u16 des);

int init()
{
  char str[] = "Welcome to NarOS !\n"; 
  char shell[] = "[root@miny1233]# cat /dev/tty";
  tty_init();
  printk(str);
  printk(shell);
  return 0x114514; //这样就能通过EAX判断内核是不是正常退出
}

