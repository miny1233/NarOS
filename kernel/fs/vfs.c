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
#include "fat.h"

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

struct file_system_type* file_system_list = NULL;
struct super_block* super_block_lists = NULL;

//注册一个文件系统
int file_system_register(struct file_system_type* filesystem)
{
    if (filesystem == NULL)
        return -1;

    struct file_system_type* node = file_system_list;
    for (;node->next != NULL;node = node->next){}

    node->next = filesystem;
    return 0;
}

static inline struct inode* open(const char* path,char mode)
{
    struct super_block* sb = super_block_lists;
    // 默认使用第一个超级快
    if (!sb)
        return NULL;
    if (!sb->s_op->open)
        return NULL;

    return super_block_lists->s_op->open(sb,path,mode);
}

int sys_mkdir(const char* path)
{
    struct super_block* sb = super_block_lists;

    return sb->s_op->mkdir(sb,path);
}

int sys_mknod(const char* path,int subtype,int nr)
{
    struct super_block* sb = super_block_lists;

    return sb->s_op->mknod(sb,path,subtype,nr);
}

// syscall
int sys_open(const char* path,char mode)
{
    // 取出fd表
    struct inode** list = running->fd;
    struct inode** newInode = NULL;
    int fd = 0;

    for (;fd < FD_NR;fd++)
    {
        if (list[fd]) {
            continue;
        }

        newInode = &list[fd];
        break;
    }
    // 打开的文件过多
    if (fd >= FD_NR)
        return -1;

    *newInode = open(path,mode);
    if (!*newInode)
        return -1;

    return fd;
}
int sys_read(int fd,char* buf,size_t len)
{
    struct inode* file = running->fd[fd];
    if (!file)
        return -1;

    if (!file->i_op->read)
        return -1;

    return file->i_op->read(file,buf,len);
}

int sys_write(int fd,const char* buf,size_t len)
{
    struct inode* file = running->fd[fd];
    if (!file)
        return -1;

    if(!file->i_op->write)
        return -1;

    return file->i_op->write(file,buf,len);
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

    // fatfs 测试
    printk("register fatfs!\n");
    file_system_register(&fatfs_type);

    // mount fatfs
    root_sb->s_op->mount(root_sb,"/home/","/dev/disk",&fatfs_type);
}
