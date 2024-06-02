//
// Created by 谢子南 on 24-6-1.
//

#include "rootfs.h"
#include "memory.h"
#include "string.h"
#include <nar/heap.h>
#include <nar/fs/fs.h>


#define R_FILE 0
#define R_DIR 1

// 先只做能显示文件名称的
struct rootfs_dir_node {
    char name[64];
};

struct rootfs_inode {
    size_t file_size;
    char type;
    void* data;
};

struct path_to_inode
{
    struct path_to_inode *next;
    char path[64];
    struct rootfs_inode* file;
};

struct inode* open(struct super_block* sb,const char* path,char mode)
{
    return NULL;
}

int mkdir(struct super_block* sb,const char* path)
{
    // 路径无效
    if (path[strlen(path) - 1] != '/')
        return -1;

    struct path_to_inode* node = (struct path_to_inode*)sb->data;
    for (;node->next != NULL;node = node->next)
    {
        if (strcmp(path,node->path) == 0)
        {
            return -1;  // 路径存在
        }
    }
    // 初始化一个指向当前目录的项目
    struct rootfs_dir_node* dir = kalloc(sizeof (struct rootfs_dir_node));
    strcpy(dir->name,".");

    // 分配元数据
    struct rootfs_inode* dir_inode = kalloc(sizeof (struct rootfs_inode));
    dir_inode->type = R_DIR;
    dir_inode->file_size = sizeof (struct rootfs_dir_node);
    dir_inode->data = dir;

    // 分配路径映射
    struct path_to_inode* new_path = kalloc(sizeof (struct path_to_inode));
    new_path->next = NULL;
    strcpy(new_path->path,path);
    new_path->file = dir_inode;

    node->next = new_path;

    return 0;
}

struct super_operations rootfs_op = {
        .mkdir = mkdir,
        .open = open,
        .close = NULL,
        .mount = NULL,
};

static struct super_block* rootfs_get_sb (struct file_system_type* fs, const char* dev_path)
{
    struct super_block* sb = kalloc(sizeof (struct super_block));
    memset(sb,0,sizeof (struct super_block));

    // 创建根
    struct path_to_inode* maps = kalloc(sizeof (struct path_to_inode));
    maps->next = NULL;
    maps->file = NULL;
    strcpy(maps->path,"\0");

    sb->data = maps;
    sb->next = NULL;
    sb->s_op = &rootfs_op;

    return sb;
}

struct file_system_type rootfs_type = {
        .next = NULL,
        .name = "rootfs",
        .get_sb = rootfs_get_sb,
        .fs_flags = 0,
        .kill_sb = NULL,
};