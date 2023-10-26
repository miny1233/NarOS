//
// Created by 谢子南 on 2023/10/26.
//

//这是对文件系统的结构要求

#ifndef NAROS_FS_H
#define NAROS_FS_H

#include <device/dev.h>

typedef struct fs{
    char name[64];  //文件系统名称
    int (*mount)(struct fs*,dev_t dev);
    int (*unmount)(struct fs*);
}fs_t;


#endif //NAROS_FS_H
