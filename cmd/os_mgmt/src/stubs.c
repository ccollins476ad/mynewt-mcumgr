/**
 * These stubs get linked in when there is no equivalent OS-specific
 * implementation.
 */

#include "mgmt/mgmt.h"
#include "os_mgmt/os_mgmt_impl.h"

int __attribute__((weak))
os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
os_mgmt_impl_reset(void)
{
    return MGMT_ERR_ENOTSUP;
}
