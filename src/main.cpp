#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <RC.h>
#include <rthw.h>
#include <string.h>

extern "C"
{
#include <dfs_fs.h>
#include "dfs_romfs.h"
}


//�ڲ�flash����
extern "C" unsigned int USER_ADDR_START;
extern "C" unsigned int USER_AREA_LEN;
extern "C" unsigned int EX_USER_ADDR_START;
extern "C" unsigned int EX_USER_AREA_LEN;
extern "C" int app_flashdev_init(void);
extern "C" int app_wlan_init();



/*
Ӳ�������жϹ���
*/
#if defined(RT_USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
extern "C"
{
    extern long list_thread(void);
}
#endif

static __attribute__((unused)) rt_err_t hw_exception_handle(void *context)
{
#if defined(RT_USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
    list_thread();
#endif

#ifdef APP_AUTORESET_ON_ERROR
    printf("hardfault occurred,system will reboot ... \r\n\r\n");
    rt_hw_cpu_reset();
#endif

    return 0;
}

#ifdef RT_DEBUG
/*
RT Thread����
*/
static __attribute__((unused)) void assert_hook(const char *ex, const char *func, rt_size_t line)
{
#if defined(RT_USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
    list_thread();
#endif

#ifdef APP_AUTORESET_ON_ERROR
    printf("%s -> %s:%d\r\nassert occurred,system will reboot ... \r\n\r\n", ex, func, (int)line);
    rt_hw_cpu_reset();
#endif

}
#endif

#ifdef APP_USING_PAHOMQTT
#include "MQTT.h"
#endif

int main(void)
{

    {
        rt_hw_exception_install(hw_exception_handle);
#ifdef RT_DEBUG
        rt_assert_set_hook(assert_hook);
#endif

    }
    /* set wifi work mode */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    rt_wlan_set_mode(RT_WLAN_DEVICE_AP_NAME, RT_WLAN_AP);
    rt_wlan_config_autoreconnect(true);

    {
        //��ӡbanner
        char *banner = (char *)RCGetHandle("banner");
        if (banner != NULL)
            printf("%s", banner);
    }

    {
        //��ӡʣ���ڴ�
        rt_size_t total = 0, used = 0;
        rt_memory_info(&total, &used, NULL);
        printf("\r\nTotal Memory:%d Bytes,Used Memory:%d Bytes\r\n", (int)total, (int)used);
    }

    {
        //�����ļ�ϵͳ,���غ��ʹ��df��echo��cat��msh�в���
        printf("User Flash Start=%08X length=%u\r\nEx User Flash Start=%08X Length=%u\r\n", USER_ADDR_START, USER_AREA_LEN, EX_USER_ADDR_START, EX_USER_AREA_LEN);
        bool is_root_ok = true;
        if (RT_EOK == app_flashdev_init())
        {
            if (EX_USER_ADDR_START != 0 && EX_USER_AREA_LEN != 0)
            {
                printf("mount ex user flash ...\r\n");
                //����exflash
                if (dfs_mount("exflash", "/", "lfs", 0, NULL) != 0)
                {
                    //���Ը�ʽ���ٹ���
                    dfs_mkfs("lfs", "exflash");
                    if (0 != dfs_mount("exflash", "/", "lfs", 0, NULL))
                    {
                        is_root_ok = false;
                        printf("mount ex user flash failed!\r\n");
                    }
                }
            }
            else
            {
                printf("mount user flash ...\r\n");
                //����flash
                if (dfs_mount("flash", "/", "lfs", 0, NULL) != 0)
                {
                    //���Ը�ʽ���ٹ���
                    dfs_mkfs("lfs", "flash");
                    if (0 != dfs_mount("flash", "/", "lfs", 0, NULL))
                    {
                        is_root_ok = false;
                        printf("mount user flash failed!\r\n");
                    }
                }
            }
        }

        if (!is_root_ok)
        {
            //��δ���ظ��ļ�ϵͳ������ʱ�ļ�ϵͳ����
            if (dfs_mount(NULL, "/", "tmp", 0, NULL) != 0)
            {
                printf("mount tmpfs on / failed!\r\n");
            }
            else
            {
                printf("mount tmpfs on / success!\r\n");
            }
        }

        //����/rom���ص�
        mkdir("/rom", 777);

        extern const struct romfs_dirent romfs_root;

        if (dfs_mount(NULL, "/rom", "rom", 0, &romfs_root) != 0)
        {
            printf("mount romfs on %s failed!\r\n", "/rom");
        }
        else
        {
            printf("mount romfs on %s success!\r\n", "/rom");
        }

    }

    {
        //����wlan����
        app_wlan_init();
    }

#ifdef APP_USING_PAHOMQTT
    MQTT_Init();
#endif

    return RT_EOK;
}
