//
// Created by 谢子南 on 2023/9/8.
//

#include <nar/fs/vfs.h>
#include <nar/panic.h>
#include <nar/printk.h>
#include <nar/fs/fs.h>
#include <device/ata.h>
#include <type.h>
#include <string.h>
#include <memory.h>
#include <nar/task.h>
#include <nar/heap.h>

// rootfs
#include "rootfs.h"

static void scan_disk()
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
}

struct file_system_type* file_system_list;
struct super_block* super_block_lists;

//注册一个文件系统
int file_system_register(struct file_system_type* filesystem)
{
    return -1;
}

void vfs_init()
{
    // 注册根文件系统
    file_system_list = &rootfs_type;
    // 挂载文件系统取得超级块
    super_block_lists = rootfs_type.get_sb(&rootfs_type,"");
    if (!super_block_lists)
    {
        LOG("rootfs mount fail!\n");
        return;
    }

    LOG("rootfs mounted!\n");
    // 创建根
    struct super_block* root_sb = super_block_lists;

    root_sb->s_op->mkdir(root_sb,"/");
    root_sb->s_op->mkdir(root_sb,"/dev/");
    root_sb->s_op->mknod(root_sb,"/dev/stdout",DEV_TTY,0);

    struct inode* tty = root_sb->s_op->open(root_sb,"/dev/stdout",0);
    tty->i_op->write(tty,"Hello World\n",0);
}

