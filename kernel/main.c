#include"printk.c"

int init()
{
  char str[] = "welcome to NarOS !";
  printk(str);
  return 0x114514; //这样就能通过EAX判断内核是不是正常退出
}

