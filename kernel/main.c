#include <type.h>
#include "printk.c"

void outb(u16 des,u8 value);
u8 inb(u16 des);

int init()
{
  tty_init();
  printk("Welcom to NarOS\n");
  printk("test test test\n");

  //滚屏测试  
  u16 count=0;
  for(char i=0;i<=10;i++)
  {   
     i%=10;
     printk("The number is :");
     char num[] = {i+'0','\n',0};
     printk(num);
     while(++count);
  }

  return 0; //这样就能通过EAX判断内核是不是正常退出
}

