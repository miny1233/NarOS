//
// Created by 谢子南 on 2023/10/26.
//

#ifndef NAROS_DEV_H
#define NAROS_DEV_H

#define DEV_NR 32 //最大设备数

#define DEV_GEN 0  // 默认设备
#define DEV_CHAR 1  // 字符设备
#define DEV_DISK 2 // 磁盘设备

#include <type.h>

// 设备管理器通过设备号通知驱动程序
struct general_dev
{
    int (*sendto)(int id,void* msg,int len);
    int (*recv)(int id,void* msg);
};

struct char_dev{
    int (*sendto)(int id,char msg);
    char (*recv)(int id);
};

struct disk_dev{
    int (*read)(int id,u32 sector,void* buf,u8 count);
    int (*write)(int id,u32 sector,const void* buf,u8 count);
};

// 设备描述
typedef struct dev_t{
    int dev_type; // 设备类型
    union {
        struct general_dev general_dev;
        struct char_dev char_dev;
        struct disk_dev disk_dev;
    };
}dev_t;


// 初始化设备管理器
void dev_manager_init();
int dev_register(dev_t dev);

#endif //NAROS_DEV_H
