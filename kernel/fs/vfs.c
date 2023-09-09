//
// Created by 谢子南 on 2023/9/8.
//

#include <nar/vfs.h>
#include <nar/printk.h>
#include "fat.h"

FATFS rootfs; //根文件系统内存

void vfs_init()
{
    printk("rootfs addr: 0x%X\n",&rootfs);
    int res = f_mount(&rootfs,"0:/",0);
    if(res == 0)printk("mount rootfs success\n");

    DIR dir;
    FILINFO fno;
    int nfile, ndir;
    //f_mkdir("/dev/");
    f_chmod("/nar", AM_SYS ,0xff);


    res = f_opendir(&dir, "/");                       /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* Directory */
                printk("<DIR>   %s attr: %x\n", fno.fname,fno.fattrib);
                ndir++;
            } else {                               /* File */
                printk("<FILE>  %d %s attr: %x\n",(unsigned int)fno.fsize,fno.fname,fno.fattrib);
                nfile++;
            }
        }
        f_closedir(&dir);
        printk("%d dirs, %d files.\n", ndir, nfile);
    } else {
        printk("Failed to open \"%s\". (%u)\n", "/", res);
    }
}