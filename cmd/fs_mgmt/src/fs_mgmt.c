/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <limits.h>
#include <string.h>

#include "fs_mgmt.h"
#include "fs_mgmt_impl.h"

/* XXX: Make these configurable. */
#define FS_MGMT_UPLOAD_CHUNK_SIZE   512
#define FS_MGMT_DOWNLOAD_CHUNK_SIZE 32
#define FS_MGMT_PATH_MAX_LEN        64

static mgmt_handler_fn fs_mgmt_download;
static mgmt_handler_fn fs_mgmt_upload;

static struct {
    uint32_t off;
    uint32_t size;
} fs_mgmt_ctxt;

static const struct mgmt_handler fs_mgmt_handlers[] = {
    [FS_MGMT_ID_FILE] = {
        .mh_read = fs_mgmt_file_download,
        .mh_write = fs_mgmt_file_upload,
    },
};

#define FS_MGMT_HANDLER_CNT                                             \
    sizeof fs_mgmt_handlers / sizeof fs_mgmt_handlers[0]

static struct mgmt_group fs_mgmt_group = {
    .mg_handlers = fs_mgmt_handlers,
    .mg_handlers_count = FS_MGMT_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_FS,
};

static int
fs_mgmt_file_download(struct mgmt_cbuf *cb)
{
    unsigned long long off = UINT_MAX;
    char path[FS_MGMT_MAX_NAME + 1];
    uint8_t file_data[FS_MGMT_DOWNLOAD_CHUNK_SIZE];
    const struct cbor_attr_t dload_attr[] = {
        {
            .attribute = "off",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
        },
        {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = path,
            .len = sizeof path,
        },
        /* XXX: Client should be able to specify length. */
        { 0 },
    };
    int rc;
    size_t file_len;
    size_t bytes_read;
    CborError err;

    rc = cbor_read_object(&cb->it, dload_attr);
    if (rc != 0 || off == UINT_MAX) {
        return MGMT_ERR_EINVAL;
    }

    if (off == 0) {
        rc = fs_mgmt_impl_filelen(path, &file_len);
        if (rc != 0) {
            return rc;
        }
    }

    rc = fs_mgmt_impl_read(name, off, FS_MGMT_DOWNLOAD_CHUNK_SIZE,
                           file_data, &bytes_read);
    if (rc != 0) {
        return rc;
    }

    err = 0;
    err |= cbor_encode_text_stringz(&cb->encoder, "off");
    err |= cbor_encode_uint(&cb->encoder, off);
    err |= cbor_encode_text_stringz(&cb->encoder, "data");
    err |= cbor_encode_byte_string(&cb->encoder, file_data, bytes_read);
    err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);

    if (off == 0) {
        err |= cbor_encode_text_stringz(&cb->encoder, "len");
        err |= cbor_encode_uint(&cb->encoder, file_len);
    }

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

static int
fs_mgmt_upload_rsp(struct mgmt_cbuf *cb, int rc, unsigned long long off)
{
    CborError err;

    err = 0;
    err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    err |= cbor_encode_int(&cb->encoder, rc);
    err |= cbor_encode_text_stringz(&cb->encoder, "off");
    err |= cbor_encode_uint(&cb->encoder, off);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

static int
fs_mgmt_file_upload(struct mgmt_cbuf *cb)
{
    uint8_t file_data[FS_MGMT_CHUNK_SIZE];
    char file_name[FS_MGMT_MAX_NAME + 1];
    size_t file_len;
    long long unsigned int off = UINT_MAX;
    long long unsigned int size = UINT_MAX;
    const struct cbor_attr_t off_attr[5] = {
        [0] = {
            .attribute = "off",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
            .nodefault = true
        },
        [1] = {
            .attribute = "data",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = file_data,
            .addr.bytestring.len = &file_len,
            .len = sizeof(file_data)
        },
        [2] = {
            .attribute = "len",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &size,
            .nodefault = true
        },
        [3] = {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = file_name,
            .len = sizeof(file_name)
        },
        [4] = { 0 },
    };
    int rc;

    rc = cbor_read_object(&cb->it, off_attr);
    if (rc || off == UINT_MAX) {
        return MGMT_ERR_EINVAL;
    }

    if (off == 0) {
        /* New upload. */
        fs_mgmt_ctxt.off = 0;
        fs_mgmt_ctxt.size = size;

        if (file_name[0] == '\0') {
            return MGMT_ERR_EINVAL;
        }
    } else if (off != fs_mgmt_ctxt.off) {
        /* Invalid offset.  Drop the data and send the expected offset. */
        return fs_mgmt_upload_rsp(MGMT_ERR_EINVAL, fs_mgmt_ctxt.off);
    }

    rc = fs_mgmt_impl_write(file_name, off, file_data, file_len);
    if (rc != 0) {
        return rc;
    }

    return fs_mgmt_upload_rsp(0, fs_mgmt_ctxt.off);
}

int
fs_mgmt_init(void)
{
    int rc;

    rc = mgmt_group_register(&fs_mgmt_group);
    return rc;
}
