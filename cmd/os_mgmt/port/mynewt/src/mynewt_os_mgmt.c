#include "hal/hal_system.h"
#include "hal/hal_watchdog.h"
#include "os/os.h"
#if MYNEWT_VAL(LOG_SOFT_RESET)
#include "reboot/log_reboot.h"
#endif
#include "os_mgmt/os_mgmt_impl.h"

static struct os_callout mynewt_os_mgmt_reset_callout;

static void
mynewt_os_mgmt_reset_tmo(struct os_event *ev)
{
    /*
     * Tickle watchdog just before re-entering bootloader.
     * Depending on what system has been doing lately, watchdog
     * timer might be close to firing.
     */
    hal_watchdog_tickle();
    hal_system_reset();
}

static uint16_t
mynewt_os_mgmt_stack_usage(const struct os_task *task)
{
    const os_stack_t *bottom;
    const os_stack_t *top;

    top = task->t_stacktop;
    bottom = task->t_stacktop - task->t_stacksize;
    while (bottom < top) {
        if (*bottom != OS_STACK_PATTERN) {
            break;
        }
        ++bottom;
    }

    return task->t_stacktop - bottom;
}

static const struct os_task *
mynewt_os_mgmt_task_at(int idx)
{
    const struct os_task *task;
    int i;

    task = STAILQ_FIRST(&g_os_task_list);
    for (i = 0; i < idx; i++) {
        if (task == NULL) {
            break;
        }

        task = STAILQ_NEXT(task, t_os_task_list);
    }

    return task;
}

int
os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info)
{
    const struct os_task *task;

    task = mynewt_os_mgmt_task_at(idx);
    if (task == NULL) {
        return MGMT_ERR_ENOENT;
    }

    out_info->oti_prio = task->t_prio;
    out_info->oti_taskid = task->t_taskid;
    out_info->oti_state = task->t_state;
    out_info->oti_stkusage = mynewt_os_mgmt_stack_usage(task);
    out_info->oti_stksize = task->t_stacksize;
    out_info->oti_cswcnt = task->t_ctx_sw_cnt;
    out_info->oti_runtime = task->t_run_time;
    out_info->oti_last_checkin = task->t_sanity_check.sc_checkin_last;
    out_info->oti_next_checkin = task->t_sanity_check.sc_checkin_last +
                                 task->t_sanity_check.sc_checkin_itvl;
    strncpy(out_info->oti_name, task->t_name, sizeof out_info->oti_name);

    return 0;
}

int
os_mgmt_impl_reset(void)
{
    int rc;

    os_callout_init(&mynewt_os_mgmt_reset_callout, mgmt_evq_get(),
                    nmgr_reset_tmo, NULL);

#if MYNEWT_VAL(LOG_SOFT_RESET)
    log_reboot(HAL_RESET_REQUESTED);
#endif
    os_callout_reset(&nmgr_reset_callout, OS_TICKS_PER_SEC / 4);

    return 0;
}
