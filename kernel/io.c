#include"type.h"
#define MAXIO 64
typedef struct {
    char buffer[1024];
    char writed;
}Buffer;
Buffer buffer[MAXIO];//最多64个io点，每个1KB缓存
size_t usingIO = 0;
struct
{
    char path[128];
    Buffer *buffer;
}PathToBuf[MAXIO];


__attribute__((fastcall)) void mount(const char* path){
    for(u32 len=0;;path++,len++)
    {
        PathToBuf[usingIO].path[len] = *path;
        if(*path=='\0')break;
    }
    PathToBuf[usingIO].buffer = &buffer[usingIO];
    PathToBuf[usingIO].buffer->writed=0;
    usingIO++;
}

__attribute__((fastcall)) void unmount(const char* path){
    return; //先不做回收了
}

__attribute__((fastcall)) int write(const char* path,void* context){
    for(int index=0;index<usingIO;index++)
    {
        const char* goal = path;
        for(char* ptr = PathToBuf[index].path;;ptr++,goal++)
        {
            if(*ptr!=*goal)goto end;;
            if(*ptr=='\0'&&*goal=='\0')break;
        }
        if(PathToBuf[index].buffer->writed == 1)return 0;
        u32 *cache = (u32*)PathToBuf[index].buffer;
        for(u32 count=0;count<256;count++,cache++){
            *cache = ((u32*)context)[count];
        }
        PathToBuf[index].buffer->writed = 1;
        return 1;
        end:
        continue;
    }
    return 0;
}
__attribute__((fastcall)) int read(const char* path,void* buffer){
    for(int index=0;index<usingIO;index++)
    {
        const char* goal = path;
        for(char* ptr = PathToBuf[index].path;;ptr++,goal++)
        {
            if(*ptr!=*goal)goto end;
            if(*ptr=='\0'&&*goal=='\0')break;
        }
        if(PathToBuf[index].buffer->writed == 0)return 0;
        u32 *cache = (u32*)PathToBuf[index].buffer;
        for(u32 count=0;count<256;count++,cache++){
            ((u32*)buffer)[count] = *cache;
        }
        PathToBuf[index].buffer->writed = 0;
        return 1;
        end:
        continue;
    }
    return 0;
}