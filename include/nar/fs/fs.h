//
// Created by 谢子南 on 2023/10/26.
//

//这是对文件系统的结构要求

#ifndef NAROS_FS_H
#define NAROS_FS_H

#include "nar/dev.h"

// fs由vfs来管理
// 即fs创建后创建者不需要保存
struct file_system_type
{
    char* name;  //文件系统名称
    int fs_flags;
    struct file_system_type* next;

    struct super_block *(*read_super) (struct super_block *, void *, int);
};

struct inode
{

};

struct super_block
{
    struct super_block* next;
    dev_t dev;
    struct file_system_type* s_type;
    struct super_operations* s_op;
};

// 超级块操作表
struct super_operations
{
    void (*read_inode) (struct inode *);        // 把磁盘中的inode数据读取入到内存中
    void (*write_inode) (struct inode *, int);  // 把inode的数据写入到磁盘中
    void (*put_inode) (struct inode *);         // 释放inode占用的内存
    void (*delete_inode) (struct inode *);      // 删除磁盘中的一个inode
    void (*put_super) (struct super_block *);   // 释放超级块占用的内存
    void (*write_super) (struct super_block *); // 把超级块写入到磁盘中

};


#endif //NAROS_FS_H
