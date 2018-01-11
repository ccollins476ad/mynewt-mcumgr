/**
 * These stubs get linked in when there is no equivalent OS-specific
 * implementation.
 */

#include "mgmt/mgmt.h"
#include "fs_mgmt/fs_mgmt_impl.h"

int  __attribute__((weak))
fs_mgmt_impl_filelen(const char *path, size_t *out_len)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
                  void *out_data, size_t *out_len)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
                   size_t len)
{
    return MGMT_ERR_ENOTSUP;
}
