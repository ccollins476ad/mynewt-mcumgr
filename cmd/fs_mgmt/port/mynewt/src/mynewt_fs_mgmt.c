#include "fs/fs.h"

int
fs_mgmt_impl_filelen(const char *path, size_t *out_len)
{
    struct fs_file *file;
    int rc;

    file = NULL;

    rc = fs_open(path, FS_ACCESS_READ, &file);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    rc = fs_filelen(file, &out_len);
    fs_close(file);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
                  void *out_data, size_t *out_len)
{
    struct fs_file *file;
    uint32_t bytes_read;
    int rc;

    file = NULL;

    rc = fs_open(path, FS_ACCESS_READ, &file);
    if (rc != 0) {
        goto done;
    }

    rc = fs_seek(file, offset);
    if (rc != 0) {
        goto done;
    }

    rc = fs_read(file, len, file_data, &bytes_read);
    if (rc != 0) {
        goto done;
    }

    *out_len = bytes_read;

done:
    if (file != NULL) {
        fs_close(file);
    }

    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
                   size_t num_bytes)
{
    struct fs_file *file;
    uint8_t access;
    int rc;
 
    access = FS_ACCESS_WRITE;
    if (offset == 0) {
        access |= FS_ACCESS_TRUNCATE;
    } else {
        access |= FS_ACCESS_APPEND;
    }

    rc = fs_open(path, access, &file);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    rc = fs_write(file, data, num_bytes);
    fs_close(file);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}
