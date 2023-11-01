//
// Created by 谢子南 on 2023/9/8.
//

#include <nar/fs/vfs.h>
#include <nar/panic.h>
#include <nar/printk.h>
#include <nar/fs/inode.h>
#include <nar/fs/fs.h>
#include <device/ata.h>
#include <type.h>
#include <string.h>
#include <memory.h>
#include <nar/task.h>

dev_t root_dev; //根设备
inode_t *root_inode;//根节点

fs_t filesystem_list[FS_LIST_SIZE];
char registered_fs[FS_LIST_SIZE];

//注册一个文件系统
int fs_register(fs_t filesystem)
{
    for(int i = 0;i < FS_LIST_SIZE ;i++) {
        if(registered_fs[i] == 0) {
            filesystem_list[i] = filesystem;
            registered_fs[i] = 1;
            return i;
        }
    }
    return -1;
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

    if(!registered_fs[0])panic("Cannot Find Root FileSystem");

    fs_t *root_fs = &filesystem_list[0];
    root_inode = root_fs->get_super_inode();    //挂载根节点
}

//暂时不支持相对寻址
int open(char* path)
{
    char* start = strsep(path);
    if(start == NULL || !root_inode->usable)return -1;
    inode_t inode_ls[128];  //一个文件夹下最多128个文件
    inode_t* pwd = root_inode;
    while(true)
    {
        start = strsep(start) + 1;
        char* end = strsep(start);
        for(char* ch = start;ch != end && *ch != 0;ch++)
        {
            if(pwd->usable && pwd->type == INODE_DIR)
            {
                memset(inode_ls,0,sizeof inode_ls);
                pwd->dir.get_inode(inode_ls);
                for(int index = 0;index < 128;index++)
                {
                    if(inode_ls[index].usable && !strcmp(inode_ls[index].name,start))
                        pwd = &inode_ls[index];
                }
            }
        }
        if(end == NULL)break;
    }
    if(!strcmp(pwd->name,start))
    {
        int fd = 0;
        for(;fd < 32;fd++) {
            if (!running->files[fd].usable) {
                running->files[fd] = *pwd;   // 拷贝元数据 到 进程控制块
                break;
            }
        }
        return fd < 32 ? fd : -1;   // 有可能出现打开文件过多的情况
    }
    return -1;
}
