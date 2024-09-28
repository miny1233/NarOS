//
// Created by 谢子南 on 24-6-7.
//
#include <type.h>
#include <unistd.h>
#include <syscall.h>

int open(const char* path,char mode)
{
    int fd;
    _syscall2(1,fd,path,mode);

    return fd;
}

int read(int fd,const char* buf,size_t buf_size)
{
    int ret;
    _syscall3(2,ret,fd,buf,buf_size);

    return ret;
}

int write(int fd,const char* buf,size_t len)
{
    int ret;
    _syscall3(3,ret, fd, buf, len);

    return ret;
}

void yield()
{
    int invalid;
    _syscall0(5, invalid);
}

void exit()
{
    int invalid;
    _syscall0(4, invalid);
}