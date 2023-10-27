//
// Created by 谢子南 on 2023/9/8.
//

#include <nar/vfs/vfs.h>
#include <nar/panic.h>
#include <nar/printk.h>
#include <nar/vfs/inode.h>
#include <nar/vfs/fs.h>
#include <device/dev.h>
#include <device/ata.h>

#include "fat/fat.h"

dev_t root_dev; //根设备

static int root_dev_read(dev_t* dev,void *buf,size_t seek,size_t size)
{
    if (dev->dev_id != 0)
        return -1;
    ata_disk_read(seek,buf,(u8)size);
    return 0;
}

static int root_dev_write(dev_t* dev,void *data,size_t seek,size_t size)
{
    if (dev->dev_id != 0)
        return -1;
    ata_disk_write(seek,data,(u8)size);
    return 0;
}

void vfs_init()
{
    //如果GRUB数据有效 则显示所有硬盘状态
    if (device_info->flags & (1<<7)) {
        LOG("got disk information\n");
        for (multiboot_disk_info *diskInfo = (void *) device_info->drives_addr;
             diskInfo < (multiboot_disk_info *) (device_info->drives_addr + device_info->drives_length);
             diskInfo = (multiboot_disk_info *) ((uint32_t) diskInfo + diskInfo->size)) {
            LOG("drive_number: %d mode:%d\n", diskInfo->disk_number, diskInfo->drive_mode);
            int i = 0;
            while (diskInfo->drive_ports[i] != 0) {
                printk("port %d : %d", diskInfo->drive_ports[i]);
            }
        }
    }

    //制作一个根设备
    root_dev.dev_id = 0;
    root_dev.dev_type = 0;
    root_dev.read = root_dev_read;
    root_dev.write = root_dev_write;

    inode_t *inode;

    //int res = f_mount(&rootfs,"0:/",0);
    //if(res == 0)LOG("mount rootfs success\n");
    //else LOG("mount rootfs fault\n");
}

//注册一个文件系统
void fs_register(fs_t* filesystem)
{

}


int open(char* path)
{

    return -1;
}
