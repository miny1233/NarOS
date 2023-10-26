//
// Created by 谢子南 on 2023/10/26.
//

#ifndef NAROS_DEV_H
#define NAROS_DEV_H

//通用设备描述
typedef struct dev{
    int dev_id; //设备号
    int dev_type; //设备类型

    //  !!! 注意 !!!
    //  在NAROS中 seek和size由创建者自行解释
    //  例如说 如果是磁盘文件 seek是扇区号，size是扇区数
    int (*read)(struct dev*,void *buf,size_t seek,size_t size);
    int (*write)(struct dev*,void *data,size_t seek,size_t size);
}dev_t;

#endif //NAROS_DEV_H
