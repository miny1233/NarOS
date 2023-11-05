//
// Created by 谢子南 on 2023/9/8.
//

#include <nar/fs/vfs.h>
#include <nar/panic.h>
#include <nar/printk.h>
#include <nar/fs/fs.h>
#include <device/ata.h>
#include <type.h>
#include <string.h>
#include <memory.h>
#include <nar/task.h>

dev_t root_dev; //根设备
fs_t *root_fs;

fs_t filesystem_list[FS_LIST_SIZE];
char registered_fs[FS_LIST_SIZE];

//注册一个文件系统
int fs_register(fs_t filesystem)
{
    for(int i = 0;i < FS_LIST_SIZE ;i++) {
        if(registered_fs[i] == 0) {
            filesystem_list[i] = filesystem;
            registered_fs[i] = 1;
            return i;
        }
    }
    return -1;
}

void vfs_init()
{
    //如果GRUB数据有效 则显示所有硬盘状态
    if (device_info->flags & (1<<7)) {
        LOG("got disk information\n");
        for (multiboot_disk_info *diskInfo = (void *) device_info->drives_addr;
             diskInfo < (multiboot_disk_info *) (device_info->drives_addr + device_info->drives_length);
             diskInfo = (multiboot_disk_info *) ((uint32_t) diskInfo + diskInfo->size)) {
            LOG("drive_number: %d mode:%d\n", diskInfo->disk_number, diskInfo->drive_mode);
            int i = 0;
            while (diskInfo->drive_ports[i] != 0) {
                printk("port %d : %d", diskInfo->drive_ports[i]);
            }
        }
    }
    // 由于不支持链接文件 所以还不支持多文件系统
    if(!registered_fs[0])panic("Cannot Find Root FileSystem");

    root_fs = &filesystem_list[0];
    root_fs->mount(root_fs, device_find(DEV_IDE_DISK,0)->dev); //挂载根设备
}

static int find_null_fd(volatile pcb_t* proc)
{
    for(int idx = 0;idx < FD_NR;idx++)
        if(!proc->files[idx].usable)
            return idx;
    return -1;
}


//暂时不支持相对寻址
int open(char* path)
{
    int fd = find_null_fd(running);
    volatile struct fd* f = &running->files[fd];

    f->file = root_fs->open(path);
    f->usable = f->file.f_type;

    // 如果是设备则提前打开设备
    if(f->file.f_type == F_DEV)
    {
        f->dev = device_find(f->file.data.dev.subtype,f->file.data.dev.idx)->dev;
    }

    return f->usable ? fd : -1;
}
