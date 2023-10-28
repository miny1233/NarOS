//
// Created by 谢子南 on 2023/10/26.
//

#ifndef NAROS_DEV_H
#define NAROS_DEV_H

#define DEV_DISK 0
#define DEV_TTY 1

#include <type.h>

//通用设备描述
typedef struct dev_t{
    int dev_id; //设备号
    int dev_type; //设备类型
    //  !!! 注意 !!!
    //  在NAROS中 seek和size由创建者自行解释
    //  例如说 如果是磁盘文件 seek是扇区号，size是扇区数
    int (*read)(struct dev_t*,void *buf,size_t seek,size_t size);
    int (*write)(struct dev_t*,void *data,size_t seek,size_t size);
}dev_t;
//  Q&A
//  Q 为什么这样设计
//  A 虽然这样设计破坏了统一性 但是如果统一为字节会发现你无法访问4GB以上的磁盘空间
//  A 对于LBA28最高120GB浪费了过多的内存

#endif //NAROS_DEV_H
