#include<type.h>

#define MAXPIPE 64 //最大管道数
__attribute__((fastcall)) void mount(const char* path);
__attribute__((fastcall)) void unmount(const char* path);
__attribute__((fastcall)) int write(const char* path,void* context);
__attribute__((fastcall)) int read(const char* path,void* buffer);