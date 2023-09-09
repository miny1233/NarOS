//
// Created by 谢子南 on 2023/9/8.
//

#ifndef NAROS_VFS_H
#define NAROS_VFS_H

#include "../../kernel/fs/fat.h"

typedef int fd_t;   //文件描述

#define N_FILE AM_ARC           //普通文件
#define N_SYS (AM_SYS | AM_ARC) //系统文件（受保护的）
#define N_DEV AM_SYS            //NAR拓展类

//虚拟文件系统启动并安装根文件系统
void vfs_init();

#endif //NAROS_VFS_H
