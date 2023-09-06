//
// Created by 谢子南 on 2023/6/23.
//

#include <nar/fs.h>
#include <bitmap.h>

#define FS_MAGIC 1233

typedef struct{
    size_t sector_size; //磁盘的总可用扇区
    size_t super_block; //超级块占用的扇区(连续)
    size_t fs_begin;    //文件系统的开始扇区
    size_t magic;       //文件系统魔数
    size_t root;        //根块
}fs_info;

struct super_block
{
    fs_info info;
    u8 bitmap[BLOCK_SIZE - sizeof(fs_info)];
}__attribute__((packed)) super_block; //根超级块（暂时不考虑多文件系统

#define CHUNK_ATB_DIR 0     //文件夹
#define CHUNK_ATB_FILE 1    //文件

#define CHUNK_T_INFO 0    //逻辑块
#define CHUNK_T_DATA 1  //数据块
typedef struct {
    int type;       // 文件类型 (文件或文件夹)
    int chunk_type;  //块类型 逻辑块纪律文件信息 数据块记录逻辑块记录不下的数据
    size_t sec;     //自己所在的扇区
    size_t next_block;  // 下一个块所在扇区
    size_t file_size;
    char file_name[32]; //文件名
}__attribute__((packed)) file_attribute;    // 文件属性

typedef union
{
    u8 data[BLOCK_SIZE - sizeof(file_attribute)];   //文件数据
    size_t ex_block[(BLOCK_SIZE - sizeof(file_attribute)) / sizeof(size_t)]; // 文件夹则执行文件夹所包含的文件
}__attribute__((packed)) chunk_data;

typedef struct
{
    file_attribute attribute;
    chunk_data data;
}__attribute__((packed)) chunk_block; // 逻辑块


//  格式化文件系统
void format(u32 sector_begin,u32 sector_end)
{
    size_t sector_size = sector_end - sector_begin + 1;
    size_t first_super_bitmap_size = (sizeof (super_block) - sizeof (fs_info)) * 8;
    size_t super_bitmap_size = BLOCK_SIZE * 8;
    size_t extern_need_block;
    for (extern_need_block = 0;
        extern_need_block * super_bitmap_size < sector_size - first_super_bitmap_size;
        extern_need_block++);
    printk("[NAFS]need extern block %d\n",extern_need_block);
    // 制作超级块
    super_block.info.sector_size = sector_size;
    super_block.info.super_block = 1 + extern_need_block;
    super_block.info.fs_begin = sector_begin;
    super_block.info.magic = FS_MAGIC;
    super_block.info.root = 0;
    memset(super_block.bitmap,0,sizeof(super_block.bitmap));

    for (int i = 0;i < super_block.info.super_block;i++)
        bitmap_set(super_block.bitmap,i,1);

    void* empty = get_page();
    memset(empty,0,512);
    for(int b=0;b < extern_need_block + 1;b++) {
        disk_write(sector_begin, empty,1); // 将超级块的磁盘清0
    }
    disk_write(sector_begin,&super_block,1);// 写入超级块
}

struct super_block* superBlock = NULL;//以后操作根超级块用这个指针

int load_super_block(u32 sector)
{
    disk_read(sector,&super_block,1);
    if(super_block.info.magic != FS_MAGIC)return -1;
    //载入超级块的位图
    //位图需要的内存页
    size_t bitmap_mem_page_size = (super_block.info.super_block / 8)
            + (super_block.info.super_block % 8 != 0);
    superBlock = get_page();
    for(int i=1;i < bitmap_mem_page_size;i++)
        get_page();

    //读取完整的根超级块
    disk_read(sector,superBlock,super_block.info.super_block);
    return 0;
}

size_t find_empty_block()
{
    for (int sec = 0;sec < superBlock->info.sector_size; sec++)
    {
        if(bitmap_get(superBlock->bitmap,sec) == 0)
            return  superBlock->info.fs_begin + sec;
    }
    return 0;
}

void set_super_bitmap(size_t sec,int data)
{
    size_t bitmap_bit = sec - superBlock->info.fs_begin;
    bitmap_set(superBlock->bitmap,bitmap_bit,data);
    disk_write(superBlock->info.fs_begin,superBlock,superBlock->info.super_block);
}

void mkroot()
{
    size_t sec = find_empty_block();
    chunk_block chunkBlock = {
            .attribute.file_size = 0,
            .attribute.sec = sec,
            .attribute.next_block = 0,
            .attribute.chunk_type = CHUNK_T_INFO,
            .attribute.type = CHUNK_ATB_DIR,
            .attribute.file_name = "root"
    };

    superBlock->info.root = sec;
    set_super_bitmap(sec,1);
    disk_write(sec,&chunkBlock,1);
}

chunk_block* chunk_open(size_t sec)
{
    chunk_block* cp = get_page();
    disk_read(sec,cp,1);
    return cp;
}
void chunk_close(chunk_block* chunk)
{
    disk_write(chunk->attribute.sec,chunk,1);
    put_page(chunk);
}

//初始化文件系统
void fs_init()
{
    // 单文件系统 默认0x100扇区启动
    size_t format_size = 10;//10MB
    format(0x100,format_size * 1024 * 2);
    if (load_super_block(0x100))
        panic("[fs] Load file system fault\n");
    //printk("[fs] load file system successful\n");
    //printk("start b:%d empty b:%d\n",superBlock->info.fs_begin,find_empty_block());
    if (superBlock->info.root == 0)
    {
        printk("[fs]cannot find root,make it now.\n");
        mkroot();
    }
    load_super_block(0x100);
    printk("[fs] find root name is %s", chunk_open(superBlock->info.root)->attribute.file_name);

}

void mkdir(const char* path)
{

}
