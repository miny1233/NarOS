//
// Created by 谢子南 on 2023/10/26.
//

//这是对文件系统的结构要求

#ifndef NAROS_FS_H
#define NAROS_FS_H

#include <nar/dev.h>

// fs由vfs来管理
// 即fs创建后创建者不需要保存
struct file_system_type
{
    char* name;  //文件系统名称
    int fs_flags;
    struct file_system_type* next;

    void* data; // 给文件系统准备的存放额外数据

    struct super_block *(*get_sb) (struct file_system_type*, const char* dev_path);
    void (*kill_sb) (struct file_system_type*,struct super_block*);
};

struct inode {

    void* data;

    struct inode_operations* i_op;
};

struct inode_operations
{
    ssize_t (*read) (struct inode *, char *, size_t);

};

struct super_block
{
    struct super_block* next;   // 链表
    dev_t dev;
    struct file_system_type* s_type;
    struct super_operations* s_op;

    void* data;
};

// 超级块操作表
struct super_operations
{
    struct inode* (*open)(struct super_block*,const char* path,char mode);
    void (*close)(struct super_block*,struct inode*);

    void (*mount)(struct super_block*,const char* mount_path,const char* dev_path,const char* fs_name);
};


#endif //NAROS_FS_H
