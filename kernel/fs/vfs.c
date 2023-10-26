//
// Created by 谢子南 on 2023/9/8.
//

#include <nar/vfs/vfs.h>
#include <nar/panic.h>
#include <nar/printk.h>
#include <nar/vfs/inode.h>
#include <nar/vfs/fs.h>

#include "fat/fat.h"

FATFS rootfs; //根文件系统内存

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

    inode_t* i;
    //挂载根文件系统
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
