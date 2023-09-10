//
// Created by 谢子南 on 2023/9/8.
//

#ifndef NAROS_VFS_H
#define NAROS_VFS_H

#include "../../kernel/fs/fat.h"
#include <nar/multiboot.h>

typedef int fd_t;   //文件描述

#define N_FILE AM_ARC           //普通文件
#define N_SYS (AM_SYS | AM_ARC) //系统文件（受保护的）
#define N_EXT AM_SYS            //NAR拓展类


//块设备
typedef struct block_dev
{
    int port;   //写入端口
    int(*write)(size_t seek,const void* buf,int len);   //向块设备写入数据
    int(*read)(size_t seek,void *buf,int len);          //从块设备读取数据
}block_dev_t;

//字符设备
typedef struct char_dev
{
    int(*write)(const char* ch);   //向块设备写入数据
    int(*read)(char* ch);          //从块设备读取数据
}char_dev_t;

#define BD 1    //块设备
#define CD 2    //字符设备
//NAR拓展文件类型
typedef struct nar_extern
{
    int n_type; //类型
    char using; //使用中
    union{
        block_dev_t block_dev;
        char_dev_t char_dev;
    };
}nar_extern_t;

/* 目前的设想是 除了主盘是锁死的 如果mount块设备
 * mount -> set disk_io -> f_mount
 * 通过设置disk_io上的号码，然后告诉f_mount应该是哪个port
 */

//虚拟文件系统启动并安装根文件系统
void vfs_init();

#endif //NAROS_VFS_H
