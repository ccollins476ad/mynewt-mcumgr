#ifndef H_FS_MGMT_IMPL_
#define H_FS_MGMT_IMPL_

int fs_mgmt_impl_filelen(const char *path, size_t *out_len);
int fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
                      void *out_data, size_t *out_len);
int fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
                       size_t num_bytes);

#endif
