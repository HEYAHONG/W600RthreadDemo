#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <RC.h>
#include <rthw.h>

/*
×¢²áflashÉè±¸
*/
#include <rtdevice.h>
#include <string.h>
#include <drv_flash.h>
#include <dfs_fs.h>

extern unsigned int USER_ADDR_START;
extern unsigned int USER_AREA_LEN;
extern unsigned int EX_USER_ADDR_START;
extern unsigned int EX_USER_AREA_LEN;

static rt_err_t _read_id(struct rt_mtd_nor_device *device)
{
    return RT_EOK;
}
static rt_ssize_t _read(struct rt_mtd_nor_device *device, rt_off_t offset, rt_uint8_t *data, rt_size_t length)
{
    rt_off_t dev_offset = offset + device->block_start * device->block_size;
    if (dev_offset + length > device->block_end * device->block_size)
    {
        return 0;
    }
    wm_flash_read(dev_offset, data, length);
    return length;
}
static rt_ssize_t _write(struct rt_mtd_nor_device *device, rt_off_t offset, const rt_uint8_t *data, rt_size_t length)
{
    rt_off_t dev_offset = offset + device->block_start * device->block_size;
    if (dev_offset + length > device->block_end * device->block_size)
    {
        return 0;
    }
    wm_flash_write(dev_offset, (void *)data, length);
    return length;
}
static rt_err_t _erase_block(struct rt_mtd_nor_device *device, rt_off_t offset, rt_uint32_t length)
{
    rt_off_t dev_offset = offset + device->block_start * device->block_size;
    if (dev_offset + length > device->block_end * device->block_size)
    {
        return RT_ERROR;
    }
    wm_flash_erase(dev_offset, length);
    return RT_EOK;
}
static struct rt_mtd_nor_driver_ops flashops =
{
    _read_id,
    _read,
    _write,
    _erase_block
};
int app_flashdev_init(void)
{
    wm_flash_init();
    if (USER_ADDR_START != 0 && USER_AREA_LEN != 0)
    {
        struct rt_mtd_nor_device *flash = (struct rt_mtd_nor_device *)rt_malloc(sizeof(struct rt_mtd_nor_device));
        memset(flash, 0, sizeof(struct rt_mtd_nor_device));

        flash->block_size = wm_flash_blksize();
        flash->block_start = (USER_ADDR_START - wm_flash_addr()) / flash->block_size;
        flash->block_end = (USER_AREA_LEN + USER_ADDR_START - wm_flash_addr()) / flash->block_size;
        flash->ops = &flashops;

        if (RT_EOK != rt_mtd_nor_register_device("flash", flash))
        {
            rt_free(flash);
            return RT_ERROR;
        }
    }

    if (EX_USER_ADDR_START != 0 && EX_USER_AREA_LEN != 0)
    {
        struct rt_mtd_nor_device *flash = (struct rt_mtd_nor_device *)rt_malloc(sizeof(struct rt_mtd_nor_device));
        memset(flash, 0, sizeof(struct rt_mtd_nor_device));

        flash->block_size = wm_flash_blksize();
        flash->block_start = (EX_USER_ADDR_START - wm_flash_addr()) / flash->block_size;
        flash->block_end = (EX_USER_AREA_LEN + EX_USER_ADDR_START - wm_flash_addr()) / flash->block_size;
        flash->ops = &flashops;

        if (RT_EOK != rt_mtd_nor_register_device("exflash", flash))
        {
            rt_free(flash);
            return RT_ERROR;
        }
    }

    return RT_EOK;
}

