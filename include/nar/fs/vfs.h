//
// Created by 谢子南 on 2023/9/8.
//

#ifndef NAROS_VFS_H
#define NAROS_VFS_H

#include "../multiboot.h"
#include "fs.h"

#define FS_LIST_SIZE 8 //最大文件系统注册个数

typedef int fd_t;   //文件描述

//虚拟文件系统启动并安装根文件系统
void vfs_init();
int fs_register(fs_t filesystem);

int open(char* path);

#endif //NAROS_VFS_H
