
#define OVERRIDE(x) $Sub$$##x

#include <stdio.h>
#include "string.h"
#include <rthw.h>
#include <rtthread.h>
#include <stdint.h>
#include "wm_type_def.h"
#include "wm_ram_config.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME  "OS_PATCH"
#define DBG_LEVEL         DBG_WARNING  
#define DBG_COLOR
#include <rtdbg.h>

/** TYPE definition of tls_os_task_t */
typedef void  *tls_os_task_t;
/** TYPE definition of tls_os_timer_t */
typedef void    tls_os_timer_t;
/** TYPE definition of tls_os_sem_t */
typedef void    tls_os_sem_t;
/** TYPE definition of tls_os_queue_t */
typedef void    tls_os_queue_t;
/** TYPE definition of tls_os_mailbox_t */
typedef void    tls_os_mailbox_t;
/** TYPE definition of tls_os_mutex_t */
typedef void    tls_os_mutex_t;
/** TYPE definition of TLS_OS_TIMER_CALLBACK */
typedef  void (*TLS_OS_TIMER_CALLBACK)(void *ptmr, void *parg);

/** MACRO definition of TIMER ONE times */
#define TLS_OS_TIMER_OPT_ONE_SHORT    1u
/** MACRO definition of TIMER PERIOD */
#define TLS_OS_TIMER_OPT_PERIOD       2u

/** ENUMERATION definition of OS STATUS */
typedef enum tls_os_status
{
    TLS_OS_SUCCESS = 0,
    TLS_OS_ERROR,
    TLS_OS_ERR_TIMEOUT,
} tls_os_status_t;

/*
由于rt_mq_recv返回值含义的变更，需重写tls_os_queue_receive
*/
tls_os_status_t OVERRIDE(tls_os_queue_receive)(tls_os_queue_t *queue,
                                     void **msg,
                                     u32 msg_size,
                                     u32 wait_time)
{
    rt_ssize_t size;
    rt_int32_t time;

    LOG_D("%s %d %p", __FUNCTION__, __LINE__, queue);
    if ((wait_time == 0) || (wait_time > RT_TICK_MAX / 2))
        time = RT_WAITING_FOREVER;
    else
        time = wait_time;

    size = rt_mq_recv(queue, msg, sizeof(void *), time);
    if (size <= 0)
    {
        //LOG_E("rt_mq_recv error %d...", err);//ex. timeout...
        return TLS_OS_ERROR;
    }
    return TLS_OS_SUCCESS;
}
