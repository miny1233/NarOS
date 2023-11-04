#include "nar/dev.h"
#include <nar/panic.h>
#include <nar/task.h>
#include "string.h"

device_t device_list[DEV_NR];

device_t *device_get(dev_t dev)
{
    assert(dev < DEV_NR);
    device_t *device = &device_list[dev];
    assert(device->type != DEV_NULL);
    return device;
}

device_t *device_find(int subtype, idx_t idx)
{
    idx_t nr = 0;   // 从设备号
    for (size_t i = 0; i < DEV_NR; i++)
    {
        device_t *device = &device_list[i];
        if(device->subtype != subtype)
            continue;
        if (nr == idx)
            return device;
        nr++;
    }
    return NULL;
}

void device_init()
{
    for(dev_t idx = 0;idx < DEV_NR;idx++)
    {
        device_t *device = &device_list[idx];
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = idx;
        device->ioctl = NULL;
        device->read = NULL;
        device->write = NULL;
        device->this_device = device;
    }

}
static device_t* get_null_device()
{
    for(dev_t idx = 0;idx < DEV_NR ;idx++)
    {
        if (device_list[idx].type == DEV_NULL)
            return &device_list[idx];
    }
    return NULL;
}

dev_t device_install(int type, int subtype,char *name,
                     void *ioctl, void *read, void *write)
{
    device_t *device = get_null_device();
    assert(device != NULL);
    device->type = type;
    device->subtype = subtype;
    strcpy(device->name, name);
    device->ioctl = ioctl;
    device->read = read;
    device->write = write;
    return device->dev;
}

int device_ioctl(dev_t dev, int cmd, void *args, int flags)
{
    device_t *device = device_get(dev);
    if (device->ioctl)
    {
        while(device->used) //设备被占用就先让出CPU时间片
            schedule();
        device->used = 1;

        int ret = device->ioctl(device->this_device, cmd, args, flags);

        device->used = 0;
        return ret;
    }
    LOG("ioctl of device %d not implemented!!!\n", dev);
    return -1;
}

int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags)
{
    device_t *device = device_get(dev);
    if (device->read)
    {
        while(device->used)
            schedule();

        device->used = 1;

        int ret = device->read(device->this_device, buf, count, idx, flags);

        device->used = 0;
        return ret;
    }
    LOG("read of device %d not implemented!!!\n", dev);
    return -1;
}

int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags)
{
    device_t *device = device_get(dev);
    if (device->write)
    {
        while(device->used)
            schedule();
        device->used = 1;

       int ret = device->write(device->this_device, buf, count, idx, flags);
       device->used = 0;
       return ret;
    }
    LOG("write of device %d not implemented!!!\n", dev);
    return -1;
}

