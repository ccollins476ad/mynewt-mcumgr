/**
 * These stubs get linked in when there is no equivalent OS-specific
 * implementation.
 */

#include "mgmt/mgmt.h"
#include "img_mgmt/img_mgmt_impl.h"

int __attribute__((weak))
img_mgmt_impl_erase_slot(void)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
img_mgmt_impl_write_pending(int slot, bool permanent)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
img_mgmt_impl_write_confirmed(void)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
img_mgmt_impl_read(int slot, unsigned int offset, void *dst,
                   unsigned int num_bytes)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
img_mgmt_impl_write_image_data(unsigned int offset, const void *data,
                               unsigned int num_bytes, bool last)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
img_mgmt_impl_swap_type(void)
{
    return MGMT_ERR_ENOTSUP;
}
