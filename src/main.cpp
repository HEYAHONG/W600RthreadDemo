#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <RC.h>
extern "C"
{
#include <oneshot.h>
}


//内部flash变量
extern "C" unsigned int USER_ADDR_START;
extern "C" unsigned int USER_AREA_LEN;
extern "C" unsigned int EX_USER_ADDR_START;
extern "C" unsigned int EX_USER_AREA_LEN;

/*
注册flash设备
*/
#include <rtdevice.h>
#include <string.h>
extern "C"
{
#include <drv_flash.h>
#include <dfs_fs.h>
}

static rt_err_t _read_id(struct rt_mtd_nor_device *device)
{
    return RT_EOK;
}
static rt_size_t _read(struct rt_mtd_nor_device *device, rt_off_t offset, rt_uint8_t *data, rt_uint32_t length)
{
    rt_off_t dev_offset = offset + device->block_start * device->block_size;
    if (dev_offset + length > device->block_end * device->block_size)
    {
        return 0;
    }
    wm_flash_read(dev_offset, data, length);
    return length;
}
static rt_size_t _write(struct rt_mtd_nor_device *device, rt_off_t offset, const rt_uint8_t *data, rt_uint32_t length)
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
static int _flashdev_init(void)
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

#ifdef APP_AUTOSTART_ONESHOT

static void app_oneshot_callback(int state, unsigned char *ssid, unsigned char *key)
{
    if (ssid != NULL)
    {
        if (rt_wlan_connect((const char *)ssid, (const char *)key) == RT_EOK)
        {
#ifdef APP_AUTOSTART_ONESHOT_AP_STOP_ON_ONESHOT_SUCCESS
            if (rt_wlan_ap_is_active())
            {
                rt_wlan_ap_stop();
            }
#endif
            wm_oneshot_stop();
        }
    }
}

#endif

int main(void)
{
    /* set wifi work mode */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    rt_wlan_set_mode(RT_WLAN_DEVICE_AP_NAME, RT_WLAN_AP);
    rt_wlan_config_autoreconnect(true);

    {
        //打印banner
        char *banner = (char *)RCGetHandle("banner");
        if (banner != NULL)
            printf("%s", banner);
    }

    {
        //打印剩余内存
        rt_size_t total = 0, used = 0;
        rt_memory_info(&total, &used, NULL);
        printf("\r\nTotal Memory:%d Bytes,Used Memory:%d Bytes\r\n", (int)total, (int)used);
    }

    {
        //挂载文件系统,挂载后可使用df、echo、cat在msh中测试
        printf("User Flash Start=%08X length=%u\r\nEx User Flash Start=%08X Length=%u\r\n", USER_ADDR_START, USER_AREA_LEN, EX_USER_ADDR_START, EX_USER_AREA_LEN);
        if (RT_EOK == _flashdev_init())
        {
            if (EX_USER_ADDR_START != 0 && EX_USER_AREA_LEN != 0)
            {
                printf("mount ex user flash ...\r\n");
                //挂载exflash
                if (dfs_mount("exflash", "/", "lfs", 0, NULL) != 0)
                {
                    //尝试格式化再挂载
                    dfs_mkfs("lfs", "exflash");
                    if (0 != dfs_mount("exflash", "/", "lfs", 0, NULL))
                    {
                        printf("mount ex user flash failed!\r\n");
                    }
                }
            }
            else
            {
                printf("mount user flash ...\r\n");
                //挂载flash
                if (dfs_mount("flash", "/", "lfs", 0, NULL) != 0)
                {
                    //尝试格式化再挂载
                    dfs_mkfs("lfs", "flash");
                    if (0 != dfs_mount("flash", "/", "lfs", 0, NULL))
                    {
                        printf("mount user flash failed!\r\n");
                    }
                }
            }
        }
    }

#ifdef APP_AUTOSTART_ONESHOT
    {
        //开启oneshot配网.
        printf("starting oneshot...\r\n");
        wm_oneshot_start((WM_ONESHOT_MODE)APP_AUTOSTART_ONESHOT_MODE, app_oneshot_callback);
        {
            //打开Wifi AP供客户端连接.设备ip由宏定义DHCPD_SERVER_IP决定,默认为192.168.169.1
            rt_wlan_start_ap(APP_AUTOSTART_ONESHOT_AP_SSID, strlen(APP_AUTOSTART_ONESHOT_AP_PASSWORD) == 0 ? NULL : APP_AUTOSTART_ONESHOT_AP_PASSWORD);
        }
    }
#endif

    return RT_EOK;
}
