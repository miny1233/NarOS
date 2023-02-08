#include"type.h"
#define MAXIO 64
char buffer[MAXIO][1024]; //最多64个io点，每个1KB缓存
size_t usingIo = 0;
struct
{
    char path[128];
    char *buffer;
}PathToBuf[MAXIO];


void mount(const char* path){
    strcpy(PathToBuf[usingIo].path[128],path);
    PathToBuf[usingIo].buffer = buffer[usingIo];
    usingIo++;
}

void unmount(const char* path){
    return;
}

void write(const char* path,void* context){

}
void read(const char* path,void* buffer){

}