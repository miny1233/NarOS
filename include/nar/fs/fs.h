//
// Created by 谢子南 on 2023/10/26.
//

//这是对文件系统的结构要求

#ifndef NAROS_FS_H
#define NAROS_FS_H

#include "nar/dev.h"

// fs由vfs来管理
// 即fs创建后创建者不需要保存
struct file_system_type{
    char* name;  //文件系统名称
    int fs_flags;
    struct file_system_type* next;

    int (*mount)(struct file_system_type*,dev_t dev);
    int (*unmount)(struct file_system_type*);

    int (*open)(const char *path);  // 打开文件

};
// 超级块操作表
struct super_operations
{
    
};

struct super_block
{
    struct super_block* next;
    dev_t dev;
    struct super_operations* s_op;
};


#endif //NAROS_FS_H
