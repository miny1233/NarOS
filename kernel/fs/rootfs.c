//
// Created by 谢子南 on 24-6-1.
//

#include "rootfs.h"
#include "memory.h"
#include <nar/heap.h>
#include <nar/fs/fs.h>


union file_or_dic
{

};

struct rootfs_inode {
    size_t file_size;
    union file_or_dic* data;
};

struct path_to_file
{
    struct path_to_file* next;
    const char* path;
    struct rootfs_inode* file;
};

struct inode* open(struct super_block* sb,const char* path,char mode)
{

}

static struct super_block* rootfs_get_sb (struct file_system_type* fs, const char* dev_path)
{
    struct super_block* sb = kalloc(sizeof (struct super_block));
    memset(sb,0,sizeof (struct super_block));

    //struct path_to_file* maps = kalloc()

    //sb->s_type = NULL;
}