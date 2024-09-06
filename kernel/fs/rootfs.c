//
// Created by 谢子南 on 24-6-1.
//

#include "rootfs.h"
#include "memory.h"
#include "string.h"
#include <nar/heap.h>
#include <math.h>
#include <nar/fs/fs.h>


#define ROOTFS_FILE 0
#define ROOTFS_DIR 1
#define ROOTFS_DEV 2
#define ROOTFS_MOUNT 3

// 先只做能显示文件名称的
struct rootfs_dir_node {
    struct rootfs_dir_node* next;

    char name[64];
};
struct rootfs_dev_node {
    int subtype;
    int nr;
    dev_t dev;
};
struct rootfs_mount_node {
    struct super_block* mount_sb;
    char prefix[64];
};

struct rootfs_inode {
    size_t file_size;
    char type;
    void* data;
};

struct path_to_inode
{
    struct path_to_inode *next;
    char path[64];
    struct rootfs_inode* file;
};

static inline int min(int a,int b)
{
    return a < b ? a : b;
}

static struct rootfs_inode* find_inode_by_path(struct super_block* sb,const char* path)
{
    const struct path_to_inode* map = sb->data;
    for (;;map = map->next)
    {
        if (strcmp(map->path,path) == 0)
        {
            return map->file;
        }
        // mount的文件只需要前缀匹配
        int prefix_len = (int)strlen(map->path);
        if (strncmp(map->path,path,prefix_len) == 0 && map->file->type == ROOTFS_MOUNT)
        {
            return map->file;
        }

        if (map->next == NULL)
        {
            return NULL;
        }
    }

}

static int add_new_inode(struct super_block* sb,const char* path,struct rootfs_inode* inode)
{
    struct path_to_inode* empty_pti = sb->data;
    for (;empty_pti->next != NULL;empty_pti = empty_pti->next);

    empty_pti->next = kalloc(sizeof (struct path_to_inode));

    if (!empty_pti->next)
        return -1;

    empty_pti = empty_pti->next;

    empty_pti->next = NULL;
    empty_pti->file = inode;
    strcpy(empty_pti->path,path);

    return 0;
}

// 元数据节点操作
static ssize_t rootfs_read(struct inode* inode,char* data, size_t len)
{
    struct rootfs_inode* rootfs_i = inode->data;

    if (rootfs_i->type != ROOTFS_DEV)
        return -1;

    struct rootfs_dev_node* dev_i = rootfs_i->data;

    if (!dev_i->dev)
        dev_i->dev = device_id_get(device_find(dev_i->subtype,dev_i->nr));

    return device_read(dev_i->dev,(void*) data,len,0,0);
}

static ssize_t rootfs_write(struct inode * inode,const char *data,size_t len)
{
    struct rootfs_inode* rootfs_i = inode->data;
    // 目前只支持读写设备
    if (rootfs_i->type != ROOTFS_DEV)
    {
        return -1;
    }

    struct rootfs_dev_node* dev_i = rootfs_i->data;
    if (!dev_i->dev)
    {
        dev_i->dev = device_id_get(device_find(dev_i->subtype,dev_i->nr));
    }

    return device_write(dev_i->dev,(void*)data,len,0,0);
}

static struct inode_operations i_op = {
        .read = rootfs_read,
        .write = rootfs_write,
};

// 超级块操作函数定义

static struct inode* rootfs_open(struct super_block* sb,const char* path,char mode)
{
    struct rootfs_inode* r_inode = find_inode_by_path(sb,path);
    if (!r_inode)
        return NULL;

    // 是mount路径则转发请求
    if (r_inode->type == ROOTFS_MOUNT)
    {
        struct rootfs_mount_node* m_n = r_inode->data;
        size_t prefix_o = strlen(m_n->prefix) - 1;

        return m_n->mount_sb->s_op->open(m_n->mount_sb,path + prefix_o,mode);
    }

    struct inode* i = kalloc(sizeof (struct inode));

    i->data = r_inode;
    i->i_op = &i_op;
    i->fno = NULL;

    return i;
}

static int rootfs_close(struct super_block* sb,struct inode* inode)
{
    kfree(inode);
    return 0;
}

static int rootfs_mkdir(struct super_block* sb,const char* path)
{
    // 路径无效
    if (path[strlen(path) - 1] != '/')
        return -1;

    struct path_to_inode* node = (struct path_to_inode*)sb->data;
    for (;;node = node->next)
    {
        if (strcmp(path,node->path) == 0)
        {
            return -1;  // 路径存在
        }

        if (node->next == NULL)
        {
            break;
        }
    }
    // 初始化一个指向当前目录的项目
    struct rootfs_dir_node* dir = kalloc(sizeof (struct rootfs_dir_node));
    dir->next = NULL;
    strcpy(dir->name,".");

    // 分配元数据
    struct rootfs_inode* dir_inode = kalloc(sizeof (struct rootfs_inode));
    dir_inode->type = ROOTFS_DIR;
    dir_inode->file_size = sizeof (struct rootfs_dir_node);
    dir_inode->data = dir;

    // 分配路径映射
    struct path_to_inode* new_path = kalloc(sizeof (struct path_to_inode));
    new_path->next = NULL;
    strcpy(new_path->path,path);
    new_path->file = dir_inode;

    node->next = new_path;

    return 0;
}

static int rootfs_mknod(struct super_block* sb,const char* path,int subtype,int nr)
{
    const char* sep = strrsep(path);
    char name[64];
    char dir[64];

    memset(name,0,sizeof name);
    memset(dir,0,sizeof dir);
    // 没有文件名称
    if (*(sep + 1) == '\0')
    {
        return -1;
    }
    // 提取路径名与文件名
    strcpy(name,sep + 1);
    memcpy(dir,path,sep - path + 1);

    // 寻找路径对应的文件夹
    //struct rootfs_inode* dir_inode = find_inode_by_path(sb,dir);
    //if (!dir_inode)
    //{
    //    return -1;  // 找不到路径
    //}

    //struct rootfs_dir_node* dir_node = dir_inode->data;
    //for(;dir_node->next != NULL;dir_node = dir_node->next);

    //创建节点映射
    //struct rootfs_dir_node* new_node = kalloc(sizeof (struct rootfs_dir_node));
    //dir_node->next = new_node;
    //strcpy(new_node->name,name);

    // 创建设备文件
    struct rootfs_dev_node* f_dev = kalloc(sizeof (struct rootfs_dev_node));
    f_dev->dev = 0; // 当有设备打开后再赋值
    f_dev->subtype = subtype;
    f_dev->nr = nr;

    struct rootfs_inode* dev_inode = kalloc(sizeof (struct rootfs_inode));
    dev_inode->type = ROOTFS_DEV;
    dev_inode->data = f_dev;
    dev_inode->file_size = sizeof (struct rootfs_inode);

    return add_new_inode(sb,path,dev_inode);
}

static int rootfs_mount(struct super_block* sb,const char* mount_path,const char* dev_path,struct file_system_type* fs)
{
    struct super_block* empty = sb;
    for (;empty->next != NULL;empty = empty->next);

    struct super_block* new_fs_sb = fs->get_sb(fs,dev_path);
    empty->next = new_fs_sb;

    struct rootfs_mount_node* mount_node = kalloc(sizeof (struct rootfs_mount_node));
    mount_node->mount_sb = new_fs_sb;
    strcpy(mount_node->prefix,mount_path);

    struct rootfs_inode* mount_i = kalloc(sizeof (struct rootfs_inode));
    mount_i->type = ROOTFS_MOUNT;
    mount_i->data = mount_node;
    mount_i->file_size = sizeof (struct rootfs_mount_node);

    return add_new_inode(sb,mount_path,mount_i);
}

static struct super_operations rootfs_op = {
        .mkdir = rootfs_mkdir,
        .mknod = rootfs_mknod,
        .open = rootfs_open,
        .close = rootfs_close,
        .mount = rootfs_mount,
};

static struct super_block* rootfs_get_sb (struct file_system_type* fs, const char* dev_path)
{
    struct super_block* sb = kalloc(sizeof (struct super_block));
    memset(sb,0,sizeof (struct super_block));

    // 创建根
    struct path_to_inode* maps = kalloc(sizeof (struct path_to_inode));
    maps->next = NULL;
    maps->file = NULL;
    strcpy(maps->path,"\0");

    sb->data = maps;
    sb->next = NULL;
    sb->s_op = &rootfs_op;

    return sb;
}

struct file_system_type rootfs_type = {
        .next = NULL,
        .name = "rootfs",
        .get_sb = rootfs_get_sb,
        .fs_flags = 0,
        .kill_sb = NULL,
};