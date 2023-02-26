#include "../include/string.h"
#include "../device/keyboard.c"
#include "printk.c"

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
  
  //滚屏测试
  char test[] = "The number is:";
  u16 count=0;
  for(char i=0;i<=10;i++)
  {   
     i%=10;
     printk(test);
     char num[] = {i+'0','\n',0};
     printk(num);
     while(++count);
  }

  return 0x114514; //这样就能通过EAX判断内核是不是正常退出
}

