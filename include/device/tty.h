#define Videos_Mem_Start 0xb8000 //显存起始
//基本输入输出
void tty_init();
void tty_write(const char* str);
void tty_clear();