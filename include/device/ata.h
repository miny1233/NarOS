//
// Created by 谢子南 on 2023/7/19.
//

#ifndef NAROS_ATA_H
#define NAROS_ATA_H

void ata_disk_read(u32 sector,void* buf,u8 count);
void ata_disk_write(u32 sector,const void* buf,u8 count);

#endif //NAROS_ATA_H
