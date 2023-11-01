#include <device/dev.h>
#include <memory.h>
#include "nar/printk.h"
#include "nar/panic.h"
#include "device/ata.h"

dev_t dev_list[DEV_NR];
char dev_map[DEV_NR];
int dev_num = 0; //设备数

// 根磁盘注册
static int disk0_drive_read(int id,u32 sector,void* buf,u8 count)
{
    assert(id == 0); //主盘一定注册在0上
    ata_disk_read(sector,buf,count);
    return 1;
}

static int disk0_drive_write(int id,u32 sector,const void* buf,u8 count)
{
    assert(id == 0); //主盘一定注册在0上
    ata_disk_write(sector,buf,count);
    return 1;
}
static int root_dev_drive_init()
{
    struct disk_dev root_disk = {
            .read = disk0_drive_read,
            .write = disk0_drive_write
    };
    dev_t root_dev = {
            .dev_type = DEV_DISK,
            .disk_dev = root_disk,
    };
    return dev_register(root_dev);
}

void dev_manager_init()
{
    // 设备树为空
    memset(dev_map,0,sizeof dev_map);

    //调用驱动注册函数
    assert(root_dev_drive_init() == 0);
}

int dev_register(dev_t dev)
{
   if (dev.dev_type > 2) // 支持的设备列表外
   {
       LOG("Unsupported Device!");
       return -1;
   }
   if (dev_num == DEV_NR)
   {
       LOG("Maximum Device");
       return -2;
   }

    int dev_id = 0;
    for(;dev_id < DEV_NR;dev_id++)
       if(dev_map[dev_id] == 0)break;

    dev_map[dev_id] = 1;
    dev_list[dev_id] = dev;
    return dev_id;
}
// 获得设备控制块
dev_t* get_dev_cb(int id)
{
    if(id < 0 || !dev_map[id])return 0;
    return &dev_list[id];
}


