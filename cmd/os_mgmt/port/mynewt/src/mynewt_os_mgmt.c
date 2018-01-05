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
