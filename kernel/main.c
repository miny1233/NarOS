#include"../device/io/tty.h"
int main()
{
  tty_init();//初始化显示设备
  char h[] = "hello";
  tty_write(h,5);
  tty_clear();
  return 100;
}