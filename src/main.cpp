#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <RC.h>
#include <rthw.h>
#include <string.h>

extern "C"
{
#include <oneshot.h>
#include <dfs_fs.h>
}


//�ڲ�flash����
extern "C" unsigned int USER_ADDR_START;
extern "C" unsigned int USER_AREA_LEN;
extern "C" unsigned int EX_USER_ADDR_START;
extern "C" unsigned int EX_USER_AREA_LEN;
extern "C" int app_flashdev_init(void);



#ifdef APP_AUTOCONFIGURE_WLAN_STA


#include <stdio.h>
#include <unistd.h>
static __attribute__((unused)) void autoconfigure_wlan_sta_load()
{
    char ssid[RT_WLAN_SSID_MAX_LENGTH + 1] = {0};
    char password[RT_WLAN_SSID_MAX_LENGTH + 1] = {0};
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_SSID, "r");
        if (fp != NULL)
        {
            fread(ssid, sizeof(ssid[0]), sizeof(ssid) - 1, fp);
            fclose(fp);
        }
    }
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD, "r");
        if (fp != NULL)
        {
            fread(password, sizeof(password[0]), sizeof(password) - 1, fp);
            fclose(fp);
        }
    }
    if (strlen(ssid))
    {
        rt_wlan_connect(ssid, strlen(password) == 0 ? NULL : password);
    }
}

static  __attribute__((unused)) void autoconfigure_wlan_sta_save(unsigned char *ssid, unsigned char *key)
{
    if (ssid != NULL)
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_SSID, "w");
        if (fp != NULL)
        {
            fwrite(ssid, 1, strlen((char *)ssid), fp);
            fclose(fp);
        }
    }

    if (key != NULL)
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD, "w");
        if (fp != NULL)
        {
            fwrite(key, 1, strlen((char *)key), fp);
            fclose(fp);
        }
    }
}

static __attribute__((unused)) void autoconfigure_wlan_sta_reset()
{
    unlink(APP_AUTOCONFIGURE_WLAN_STA_SSID);
    unlink(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD);
}

#ifdef RT_USING_FINSH
static int cmd_reset_autoconfigure_wlan(int argc, char **argv)
{
    autoconfigure_wlan_sta_reset();
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_reset_autoconfigure_wlan, reset_wlan_cfg, Reset wlan config);
#endif

#endif


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
#ifdef APP_AUTOCONFIGURE_WLAN_STA
            autoconfigure_wlan_sta_save(ssid, key);
#endif
        }
    }
}

#endif


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
                        printf("mount user flash failed!\r\n");
                    }
                }
            }
        }
    }

#ifdef APP_AUTOCONFIGURE_WLAN_STA
    autoconfigure_wlan_sta_load();
#endif

#ifdef APP_AUTOSTART_ONESHOT
    if (!rt_wlan_is_connected())
    {
        //����oneshot����.
        printf("starting oneshot...\r\n");
        wm_oneshot_start((WM_ONESHOT_MODE)APP_AUTOSTART_ONESHOT_MODE, app_oneshot_callback);
        {
            //��Wifi AP���ͻ�������.�豸ip�ɺ궨��DHCPD_SERVER_IP����,Ĭ��Ϊ192.168.169.1
            rt_wlan_start_ap(APP_AUTOSTART_ONESHOT_AP_SSID, strlen(APP_AUTOSTART_ONESHOT_AP_PASSWORD) == 0 ? NULL : APP_AUTOSTART_ONESHOT_AP_PASSWORD);
        }
    }
#endif

#ifdef APP_USING_PAHOMQTT
    MQTT_Init();
#endif

    return RT_EOK;
}
