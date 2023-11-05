//
// Created by 谢子南 on 2023/10/26.
//


// inode与子结构体 定义
// 用于描述文件类型与操作
// inode可以是文件或者文件夹
#ifndef NAROS_INODE_H
#define NAROS_INODE_H

// 元数据
typedef struct inode {
    int iid;        // inode编号
    int(*rm)(struct inode*);
    //读写
    int (*lseek)();
    int(*read)(struct inode*,void* buf,int size);
    int(*write)(struct inode*,void* buf,int size);
}inode_t;

union file{
        char name[64];  // 名称
        u8 f_mod;
    struct{
        inode_t *inode; // 文件open后才有操作相关
    } file;
    struct
    {
        int subtype;    // 主设备号
        int idx;        // 从设备号
    } dev;

};

//这里是目前支持的文件类型
#define FD_NULL 0  // 空文件
#define FD_FILE 1  // 普通文件
#define FD_DEV 2   // 设备文件

struct fd{
    int type;       // 类型
    union file file;
};

#endif //NAROS_INODE_H
