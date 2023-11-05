//
// Created by 谢子南 on 2023/10/26.
//

//这是对文件系统的结构要求

#ifndef NAROS_FS_H
#define NAROS_FS_H

#include "nar/dev.h"

// 元数据
typedef struct inode {
    int iid;        // inode编号
    int(*rm)(struct inode*);
    //读写
    int (*lseek)();
    int(*read)(struct inode*,void* buf,int size);
    int(*write)(struct inode*,void* buf,int size);
}inode_t;

#define F_NULL 0  // 空文件
#define F_FILE 1  // 普通文件
#define F_DIR
#define F_DEV 2   // 设备文件
struct file{
    char name[64];  // 名称
    int f_type;       // 文件类型 （类似从类型 文件 文件夹）
    u8 f_mod;
    union{
        struct {
            inode_t inode; // 文件操作
        } file;
        struct {
            int subtype;    // 主设备号
            int idx;        // 从设备号
        } dev;
    }data;
};

struct fd{
    u8 usable;    //  可用
    struct file file;
    dev_t dev;      // 缓存的设备号
};


// fs由vfs来管理
// 即fs创建后创建者不需要保存
typedef struct fs{
    char name[64];  //文件系统名称
    int (*mount)(struct fs*,dev_t dev);
    int (*unmount)(struct fs*);

    struct file (*open)(const char *path);
}fs_t;


#endif //NAROS_FS_H
