//
// Created by 谢子南 on 2023/9/5.
//

#ifndef NAROS_FS_H
#define NAROS_FS_H

#include "type.h"
#include <device/tty.h>
#include <nar/printk.h>
#include <device/ata.h>
#include <memory.h>
#include <nar/mem.h>
#include <nar/panic.h>

#define BLOCK_SIZE 512 //块大小

//设置文件系统的开始扇区
void fs_init();

#endif //NAROS_FS_H
