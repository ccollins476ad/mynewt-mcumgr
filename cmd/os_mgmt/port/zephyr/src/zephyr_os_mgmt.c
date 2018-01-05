#include <zephyr.h>
#include <misc/reboot.h>
#include "os_mgmt/os_mgmt_impl.h"

static void zephyr_os_mgmt_reset_cb(struct k_timer *timer);

static K_TIMER_DEFINE(zephyr_os_mgmt_reset_timer,
                      zephyr_os_mgmt_reset_cb, NULL);


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
