#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <RC.h>
#include <rthw.h>
#include <string.h>
#include <oneshot.h>

#ifdef APP_AUTOCONFIGURE_WLAN_STA


#include <stdio.h>
#include <unistd.h>
static __attribute__((unused)) void autoconfigure_wlan_sta_load()
{
    char ssid[RT_WLAN_SSID_MAX_LENGTH + 1] = {0};
    char password[RT_WLAN_PASSWORD_MAX_LENGTH + 1] = {0};
    {
        size_t ssid_len = strlen(APP_AUTOCONFIGURE_WLAN_STA_SSID_FAILBACK);
        if (ssid_len > RT_WLAN_SSID_MAX_LENGTH)
        {
            ssid_len =   RT_WLAN_SSID_MAX_LENGTH;
        }
        memcpy(ssid, APP_AUTOCONFIGURE_WLAN_STA_SSID_FAILBACK, ssid_len);
        size_t password_len = strlen(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD_FAILBACK);
        if (password_len > RT_WLAN_PASSWORD_MAX_LENGTH)
        {
            password_len =   RT_WLAN_PASSWORD_MAX_LENGTH;
        }
        memcpy(password, APP_AUTOCONFIGURE_WLAN_STA_PASSWORD_FAILBACK, password_len);
    }
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_SSID_FILE_PATH, "r");
        if (fp != NULL)
        {
            fread(ssid, sizeof(ssid[0]), sizeof(ssid) - 1, fp);
            fclose(fp);
        }
    }
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD_FILE_PATH, "r");
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
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_SSID_FILE_PATH, "w");
        if (fp != NULL)
        {
            fwrite(ssid, 1, strlen((char *)ssid), fp);
            fclose(fp);
        }
    }

    if (key != NULL)
    {
        FILE *fp = fopen(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD_FILE_PATH, "w");
        if (fp != NULL)
        {
            fwrite(key, 1, strlen((char *)key), fp);
            fclose(fp);
        }
    }
}

static __attribute__((unused)) void autoconfigure_wlan_sta_reset()
{
    unlink(APP_AUTOCONFIGURE_WLAN_STA_SSID_FILE_PATH);
    unlink(APP_AUTOCONFIGURE_WLAN_STA_PASSWORD_FILE_PATH);
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

int app_wlan_init()
{
#ifdef APP_AUTOCONFIGURE_WLAN_STA
    autoconfigure_wlan_sta_load();
#endif

#ifdef APP_AUTOSTART_ONESHOT
    if (!rt_wlan_is_connected())
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
}
