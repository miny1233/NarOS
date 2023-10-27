//
// Created by 谢子南 on 2023/10/26.
//
// inode与子结构体 定义
// 用于描述文件类型与操作
// inode可以是文件或者文件夹
#ifndef NAROS_INODE_H
#define NAROS_INODE_H

//这里是目前支持的文件类型
#define FILE 0  // 普通文件
#define DIR 1   // 文件夹
#define DEV 2   // 设备文件

typedef struct inode {
    char name[64];  // 名称
    int type;       // 类型
    int iid;        // inode编号
    // 删除元数据和文件
    int(*rm)(struct inode*);
    //文件夹和文件的操作是不同的
    union {
        struct {
            //文件读写
            int(*read)(struct inode*,void* buf,int seek,int size);
            int(*write)(struct inode*,void* buf,int seek,int size);
        } file;
        struct {
            //获取文件夹的所有元数据
            int (*get_inode)(struct inode *inode_list);
        } dir;
    };
}inode_t;

#endif //NAROS_INODE_H
