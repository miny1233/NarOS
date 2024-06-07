//
// Created by 谢子南 on 2023/9/8.
//

#ifndef NAROS_VFS_H
#define NAROS_VFS_H

#include "../multiboot.h"
#include "fs.h"

#define FS_LIST_SIZE 8 //最大文件系统注册个数

typedef int fd_t;   //文件描述

// 虚拟文件系统启动并安装根文件系统
void vfs_init();
int file_system_register(struct file_system_type* filesystem);

// syscall
int sys_mknod(const char* path,int subtype,int nr);
int sys_mkdir(const char* path);

int sys_open(const char* path,char mode);
int sys_read(int fd,char* buf,size_t len);
int sys_write(int fd,const char* buf,size_t len);

#endif //NAROS_VFS_H
