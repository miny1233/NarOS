#include<type.h>

#define pipe_num 64

typedef int pipe_t;
//本来就是实验而已，就不考虑性能了
void pipe_init(); //给init程序使用，用于启动管道系统

pipe_t pipe_create(void* buffer);//创建方提供缓存

int pipe_destory(pipe_t pipe);

int pipe_close(pipe_t pipe);

int pipe_open(pipe_t pipe); //调用方需要先打开流才能写入，否则操作系统拒绝写入请求

size_t pipe_wirte(pipe_t pipe,void* buffer,size_t len);//只允许写就能保证内存安全

