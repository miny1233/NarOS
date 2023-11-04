//
// Created by 谢子南 on 2023/10/26.
//

#ifndef NAROS_DEV_H
#define NAROS_DEV_H

#include "../type.h"

#define DEV_NR 32 //最大设备数

#define DEV_NULL 0  // NULL设备
#define DEV_CHAR 1  // 字符设备
#define DEV_BLOCK 2 // 块设备

// 设备子类型
enum device_subtype_t
{
    DEV_KEYBOARD = 1,    // 键盘
    DEV_TTY,         // TTY 设备
    DEV_IDE_DISK,    // IDE 磁盘
};

// ioctl
#define DEV_CMD_SECTOR_START  1 // 获得设备扇区开始位置 lba
#define DEV_CMD_SECTOR_COUNT 2  // 获得设备扇区数量
#define DEV_CMD_SECTOR_SIZE 3   // 获得设备扇区大小

typedef int dev_t;

typedef struct device_t
{
    char name[64];  // 设备名
    int type;            // 设备类型 (给内核区分操作方法)
    int subtype;         // 设备子类型 (相当于主设备号)
    dev_t dev;           // 设备号
    int used;            // 设备被占用
    struct device_t *this_device;   // this指针 可参考C++成员函数的调用原理
    // 设备控制
    int (*ioctl)(void *dev, int cmd, void *args, int flags);
    // 读设备
    int (*read)(void *dev, void *buf, size_t count, idx_t idx, int flags);
    // 写设备
    int (*write)(void *dev, void *buf, size_t count, idx_t idx, int flags);
} device_t;

//安装设备
void device_init();

dev_t device_install(int type, int subtype,char *name,
                     void *ioctl, void *read, void *write);

//主设备号 与 从设备号
device_t *device_find(int subtype, idx_t idx);

int device_ioctl(dev_t dev, int cmd, void *args, int flags);
int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags);
int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags);



// 初始化设备管理器

#endif //NAROS_DEV_H
