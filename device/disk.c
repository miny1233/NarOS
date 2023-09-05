#include <type.h>
#include <device/disk.h>
#include <device/io.h>
#include <nar/task.h>
#include <nar/panic.h>

#define DISKREAD 0
#define DISKWRITE 1

// https://wiki.osdev.org/PCI_IDE_Controller

#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

int ata_state_handle()
{
    u16 addr = 0x01f7;
    u8 status;
    while(true) {
        //yield();    // 硬盘速度是比较慢的 可以先放弃cpu时间
        status = inb(addr);
        if (status & ATA_SR_BSY) {
            continue;
        }
        if (status & ATA_SR_DRQ)
            return 0;
        if (status & ATA_SR_DRDY)
            return 0;
        if (status & ATA_SR_DSC)
            return 0;
        if (status & ATA_SR_ERR) {
            printk("Disk Error");
            return -1;
        }
        if (status & ATA_SR_DF)
        {
            printk("Device Error");
            return -2;
        }
        if (status & ATA_SR_CORR) {
            printk("Corrected data");
        }
        if (status & ATA_SR_IDX) {
            printk("Index");
        }
    }

}

void disk_rw(u32 sector,void* buf,u8 count,int mode)
{
    u8 status;

    outb(0x01f1,0x00);
    //这里踩了一个坑，发送NULL是让控制器准备，只有准备完毕才能继续
    if(ata_state_handle())return;

    u16 addr = 0x01f2;
    outb(addr++,count); //读取扇区数

    for(int i = 0;i < 3;i++)    //  循环拆分来取字节
    {
        outb(addr++,sector & 0xff);
        sector>>=8;
    }
    assert(addr == 0x01f6);
    outb(addr++,0xe0 | (sector & 0xf));
    outb(addr,mode == DISKREAD ? ATA_CMD_READ_PIO : ATA_CMD_WRITE_PIO); //读写命令

    if(ata_state_handle())return;

    int read_count = (count * 512) / 2;//每个扇区是512字节 一次读2字节

    u16* buffer = buf;
    while(read_count--)
    {
        if(mode == DISKREAD)
            *buffer++ = inw(0x01f0);
        else if(mode == DISKWRITE)
            outw(0x01f0,*buffer++);
    }
}

void disk_read(u32 sector,void* buf,u8 count)
{
    disk_rw(sector,buf,count,DISKREAD);
}

void disk_write(u32 sector,void* buf,u8 count)
{
    disk_rw(sector,buf,count,DISKWRITE);
}
