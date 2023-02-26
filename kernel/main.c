#include"printk.c"
#include "../include/string.h"

int offset;
char tmp[128];

int init()
{
  char str[] = "Welcome to NarOS !\n"; 
  char shell[] = "[root@miny1233]# ";
  tty_init();
  printk(str);
  printk(shell);
  strcpy(tmp,str);
  printk(tmp);
  return 0x114514; //这样就能通过EAX判断内核是不是正常退出
}

