#include <type.h>
#include <device/disk.h>
#include <device/io.h>
#include <nar/task.h>
#include <nar/panic.h>

void disk_read(u32 sector,void* buf,u8 count)
{
    u16 addr = 0x01f2;
    outb(addr++,count); //读取扇区数

    for(int i = 0;i < 3;i++)    //  循环拆分来取字节
    {
        outb(addr++,sector & 0xff);
        sector>>=8;
    }
    assert(addr == 0x01f6);
    outb(addr++,0xe0 | (sector & 0xf));
    outb(addr,0x20); //读命令

    u8 status;
    do{
        //yield();    // 硬盘速度是比较慢的 可以先放弃cpu时间
        status = inb(addr);
    }while((status & 0x88) != 0x08);

    int read_count = (count * 512) / 2;//每个扇区是512字节 一次读2字节

    u16* buffer = buf;
    while(read_count--)
    {
        u16 data = inw(0x01f0);
        *buffer++ = data;
    }

}