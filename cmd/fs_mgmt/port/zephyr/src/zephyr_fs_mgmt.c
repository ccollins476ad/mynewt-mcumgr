#include "mgmt/mgmt.h"
#include "fs_mgmt/fs_mgmt_impl.h"
#include "fs.h"

int
fs_mgmt_impl_filelen(const char *path, size_t *out_len)
{
    struct fs_dirent dirent;
    int rc;

    rc = fs_stat(path, &dirent);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    *out_len = dirent.size;

    return 0;
}

int
fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
                  void *out_data, size_t *out_len)
{
    fs_file_t file;
    ssize_t bytes_read;
    int rc;

    rc = fs_open(&file, path);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    rc = fs_seek(&file, offset, FS_SEEK_SET);
    if (rc != 0) {
        goto done;
    }

    bytes_read = fs_read(&file, out_data, len);
    if (bytes_read < 0) {
        goto done;
    }

    *out_len = bytes_read;

done:
    fs_close(&file);

    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
                   size_t len)
{
    fs_file_t file;
    int rc;
 
    if (offset == 0) {
        fs_unlink(path);
    }

    rc = fs_open(&file, path);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    rc = fs_seek(&file, offset, FS_SEEK_SET);
    if (rc != 0) {
        goto done;
    }

    rc = fs_write(&file, data, len);
    if (rc < 0) {
        goto done;
    }

    rc = 0;

done:
    fs_close(&file);

    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}
