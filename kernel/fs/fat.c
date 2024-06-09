//
// Created by 谢子南 on 24-6-9.
//

#include "fat.h"
#include "fat/fat.h"
#include <nar/heap.h>

ssize_t fat_read (struct inode *i , char *buf, size_t buf_size)
{
    FIL* file = i->data;
    size_t read_len = 0;

    f_read(file,buf,buf_size,&read_len);

    return (ssize_t)read_len;
}

static struct inode_operations fat_i_op = {
    .read = fat_read,
    .write = NULL,
};

static struct inode* fat_open(struct super_block* sb,const char* path,char _mode)
{
    // mode 转换
    const char mode = _mode;
    char fat_mode = 0;
    if (O_RDONLY & mode)
        fat_mode |= FA_READ;
    if (O_WRONLY & mode)
        fat_mode |= FA_WRITE;
    if (O_APPEND & mode)
        fat_mode |= FA_OPEN_APPEND;
    if (O_CREAT & mode)
        fat_mode |= FA_OPEN_ALWAYS;
    if (O_TRUNC & mode)
        fat_mode |= FA_CREATE_ALWAYS;

    FIL* file = kalloc(sizeof (FIL));
    int ret = f_open(file,path,fat_mode);

    if (ret)
    {
        kfree(file);
        return NULL;
    }

    struct inode* fd = kalloc(sizeof (struct inode));
    fd->i_op = &fat_i_op;
    fd->fno = NULL;
    fd->data = file;

    return fd;
}

struct super_operations fat_s_op = {
    .open = fat_open,
    .close = NULL,
    .mkdir = NULL,
    .mknod = NULL,
    .mount = NULL,
};

static struct super_block* fat_get_sb(struct file_system_type* fs, __attribute__((unused)) const char* dev_path)
{
    struct super_block* fat_sb = kalloc(sizeof (struct super_block));
    FATFS *fatfs = kalloc(sizeof (FATFS));

    int ret = f_mount(fatfs,"/",1);

    if (ret != FR_OK)
    {
        kfree(fat_sb);
        kfree(fatfs);
        return NULL;
    }

    fat_sb->dev = 0;   // 目前文件系统写死了
    fat_sb->next = NULL;
    fat_sb->data = fatfs;
    fat_sb->s_type = fs;
    fat_sb->s_op = &fat_s_op;

    return fat_sb;
}

struct file_system_type fatfs_type = {
        .kill_sb = NULL,
        .name = "fat",
        .get_sb = fat_get_sb,
};