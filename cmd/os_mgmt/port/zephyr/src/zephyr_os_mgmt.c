#include <zephyr.h>
#include <misc/reboot.h>
#include <debug/object_tracing.h>
#include <kernel_structs.h>
#include "mgmt/mgmt.h"
#include "os_mgmt/os_mgmt.h"
#include "os_mgmt/os_mgmt_impl.h"

static void zephyr_os_mgmt_reset_cb(struct k_timer *timer);

static K_TIMER_DEFINE(zephyr_os_mgmt_reset_timer,
                      zephyr_os_mgmt_reset_cb, NULL);

#ifdef CONFIG_THREAD_MONITOR
static const struct k_thread *
zephyr_os_mgmt_task_at(int idx)
{
    const struct k_thread *thread;
    int i;

    thread = SYS_THREAD_MONITOR_HEAD;
    for (i = 0; i < idx; i++) {
        if (thread == NULL) {
            break;
        }
        thread = SYS_THREAD_MONITOR_NEXT(thread);
    }

    return thread;
}

int
os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info)
{
    const struct k_thread *thread;

    thread = zephyr_os_mgmt_task_at(idx);
    if (thread == NULL) {
        return MGMT_ERR_ENOENT;
    }

    *out_info = (struct os_mgmt_task_info){ 0 };

    snprintf(out_info->oti_name, sizeof out_info->oti_name, "%d",
             thread->base.prio);
    out_info->oti_prio = thread->base.prio;
    out_info->oti_taskid = idx;
    out_info->oti_state = thread->base.thread_state;
    out_info->oti_stksize = thread->stack_info.size / 4;

    return 0;
}
#endif /* CONFIG_THREAD_MONITOR */

static void
zephyr_os_mgmt_reset_cb(struct k_timer *timer)
{
    sys_reboot(SYS_REBOOT_WARM);
}

int
os_mgmt_impl_reset(void)
{
    k_timer_start(&zephyr_os_mgmt_reset_timer, K_MSEC(250), 0);

    return 0;
}
