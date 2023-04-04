#include <nar/pipe.h>

struct{
    char used;
    char lock;
    void* buffer;
}pipe_list[64];

void pipe_init()
{
    for(int i=0;i<pipe_num;i++)
    {
      pipe_list[i].used = 0;
      pipe_list[i].lock = 0; //这个地方应该记录PID，但是进程调度还没弄好
      pipe_list[i].buffer = NULL;
    }
}

pipe_t pipe_creat(void *buffer)
{
    for(pipe_t pd=0;pd<pipe_num;pd++)
    {
        if(pipe_list[pd].used == 0)
        {
            pipe_list[pd].used=1;
            return pd;
        }
    }
    return -1;
}

int pipe_des(pipe_t pipe)
{
    if(pipe>=(unsigned int)pipe_num)return -1;
    pipe_list[pipe].buffer = NULL;
    pipe_list[pipe].lock = 0;
    pipe_list[pipe].used = 0;
    return pipe;
}

int pipe_close(pipe_t pipe)
{
    if(pipe>=(unsigned int)pipe_num)return -1;//转uint可以防止通过负数造成内存越界
    pipe_list[pipe].lock = 0;
    return pipe;
}

int pipe_open(pipe_t pipe)
{
    if(pipe>=(unsigned int)pipe_num)return -1;
    if(pipe_list[pipe].used == 0 || pipe_list[pipe].lock == 1)return -1;//未使用或者已占用
    pipe_list[pipe].lock = 1; //成功，上锁
    return pipe;
}

size_t pipe_wirte(pipe_t pipe, void *buffer, size_t len)
{
    size_t written = 0;
    do{
        ((char*)pipe_list[pipe].buffer)[written] = ((char*)buffer)[written];
    }while(++written!=len);
    return written;
}