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
#include <assert.h>
#include <string.h>

#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"

#include "img/img.h"
#include "img/image.h"
#include "img/img_impl.h"
#include "img_priv.h"

#define IMG_MAX_CHUNK_SIZE 512

static int img_upload(struct mgmt_cbuf *);
static int img_erase(struct mgmt_cbuf *);

static const struct mgmt_handler img_nmgr_handlers[] = {
    [IMG_NMGR_ID_STATE] = {
        .mh_read = img_state_read,
        .mh_write = img_state_write,
    },
    [IMG_NMGR_ID_UPLOAD] = {
        .mh_read = NULL,
        .mh_write = img_upload
    },
    [IMG_NMGR_ID_ERASE] = {
        .mh_read = NULL,
        .mh_write = img_erase
    },
#if 0
    [IMG_NMGR_ID_CORELIST] = {
#if MYNEWT_VAL(IMG_COREDUMP)
        .mh_read = img_core_list,
        .mh_write = NULL
#else
        .mh_read = NULL,
        .mh_write = NULL
#endif
    },
    [IMG_NMGR_ID_CORELOAD] = {
#if MYNEWT_VAL(IMG_COREDUMP)
        .mh_read = img_core_load,
        .mh_write = img_core_erase,
#else
        .mh_read = NULL,
        .mh_write = NULL
#endif
    },
#endif
};

#define IMG_HANDLER_CNT                                                \
    sizeof(img_nmgr_handlers) / sizeof(img_nmgr_handlers[0])

static struct mgmt_group img_nmgr_group = {
    .mg_handlers = (struct mgmt_handler *)img_nmgr_handlers,
    .mg_handlers_count = IMG_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_IMAGE,
};

static struct {
    off_t off;
    size_t image_len;
    bool uploading;
} img_ctxt;

static int
img_img_tlvs(const struct image_header *hdr,
             int slot, off_t *start_off, off_t *end_off)
{
    struct image_tlv_info tlv_info;
    int rc;

    rc = img_impl_read(slot, *start_off, &tlv_info, sizeof tlv_info);
    if (rc != 0) {
        return -1;
    }

    if (tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC) {
        return 1;
    }

    *start_off += sizeof tlv_info;
    *end_off = *start_off + tlv_info.it_tlv_tot;

    return 0;
}

/*
 * Read version and build hash from image located slot "image_slot".  Note:
 * this is a slot index, not a flash area ID.
 *
 * @param image_slot
 * @param ver (optional)
 * @param hash (optional)
 * @param flags
 *
 * Returns -1 if area is not readable.
 * Returns 0 if image in slot is ok, and version string is valid.
 * Returns 1 if there is not a full image.
 * Returns 2 if slot is empty. XXXX not there yet
 * XXX Define return code macros.
 */
int
img_read_info(int image_slot, struct image_version *ver, uint8_t *hash,
              uint32_t *flags)
{
    struct image_header hdr;
    struct image_tlv tlv;
    uint32_t data_off;
    uint32_t data_end;
    int rc;

    rc = img_impl_read(image_slot, 0, &hdr, sizeof hdr);
    if (rc != 0) {
        return -1;
    }

    if (ver != NULL) {
        memset(ver, 0xff, sizeof(*ver));
    }
    if (hdr.ih_magic == IMAGE_MAGIC) {
        if (ver != NULL) {
            memcpy(ver, &hdr.ih_ver, sizeof(*ver));
        }
    } else if (hdr.ih_magic == 0xffffffff) {
        return 2;
    } else {
        return 1;
    }

    if (flags != NULL) {
        *flags = hdr.ih_flags;
    }

    /* The hash is contained in a TLV after the image. */
    data_off = hdr.ih_hdr_size + hdr.ih_img_size;
    rc = img_img_tlvs(&hdr, image_slot, &data_off, &data_end);
    if (rc != 0) {
        return rc;
    }

    while (data_off + sizeof tlv <= data_end) {
        rc = img_impl_read(image_slot, data_off, &tlv, sizeof tlv);
        if (rc != 0) {
            return 0;
        }
        if (tlv.it_type == 0xff && tlv.it_len == 0xffff) {
            return 1;
        }
        if (tlv.it_type != IMAGE_TLV_SHA256 || tlv.it_len != IMG_HASH_LEN) {
            data_off += sizeof tlv + tlv.it_len;
            continue;
        }
        data_off += sizeof tlv;
        if (hash != NULL) {
            if (data_off + IMG_HASH_LEN > data_end) {
                return 0;
            }
            rc = img_impl_read(image_slot, data_off, hash, IMG_HASH_LEN);
            if (rc != 0) {
                return 0;
            }
        }
        return 0;
    }

    return 1;
}

/*
 * Finds image given version number. Returns the slot number image is in,
 * or -1 if not found.
 */
int
img_find_by_ver(struct image_version *find, uint8_t *hash)
{
    int i;
    struct image_version ver;

    for (i = 0; i < 2; i++) {
        if (img_read_info(i, &ver, hash, NULL) != 0) {
            continue;
        }
        if (!memcmp(find, &ver, sizeof(ver))) {
            return i;
        }
    }
    return -1;
}

/*
 * Finds image given hash of the image. Returns the slot number image is in,
 * or -1 if not found.
 */
int
img_find_by_hash(uint8_t *find, struct image_version *ver)
{
    int i;
    uint8_t hash[IMG_HASH_LEN];

    for (i = 0; i < 2; i++) {
        if (img_read_info(i, ver, hash, NULL) != 0) {
            continue;
        }
        if (!memcmp(hash, find, IMG_HASH_LEN)) {
            return i;
        }
    }
    return -1;
}

static int
img_erase(struct mgmt_cbuf *cb)
{
    CborError err;
    int rc;

    rc = img_impl_erase_slot();

    err = 0;
    err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    err |= cbor_encode_int(&cb->encoder, rc);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

static int
img_write_upload_rsp(struct mgmt_cbuf *cb, int status)
{
    CborError err;

    err = 0;
    err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    err |= cbor_encode_int(&cb->encoder, status);
    err |= cbor_encode_text_stringz(&cb->encoder, "off");
    err |= cbor_encode_int(&cb->encoder, img_ctxt.off);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;
}

/* XXX: Rename */
static int
img_upload_first(struct mgmt_cbuf *cb, const uint8_t *req_data, size_t len)
{
    struct image_header hdr;
    int rc;

    if (len < sizeof hdr) {
        return MGMT_ERR_EINVAL;
    }

    memcpy(&hdr, req_data, sizeof hdr);
    if (hdr.ih_magic != IMAGE_MAGIC) {
        return MGMT_ERR_EINVAL;
    }

    if (img_state_slot_in_use(1)) {
        /* No free slot. */
        return MGMT_ERR_ENOMEM;
    }

    rc = img_impl_erase_slot();
    if (rc != 0) {
        return rc;
    }

    img_ctxt.uploading = true;
    img_ctxt.off = 0;
    img_ctxt.image_len = 0;

    return 0;
}

static int
img_upload(struct mgmt_cbuf *cb)
{
    long long unsigned int off = UINT_MAX;
    long long unsigned int size = UINT_MAX;
    uint8_t img_data[IMG_MAX_CHUNK_SIZE];
    size_t data_len = 0;
    bool last;
    int rc;

    const struct cbor_attr_t off_attr[4] = {
        [0] = {
            .attribute = "data",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = img_data,
            .addr.bytestring.len = &data_len,
            .len = sizeof(img_data)
        },
        [1] = {
            .attribute = "len",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &size,
            .nodefault = true
        },
        [2] = {
            .attribute = "off",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
            .nodefault = true
        },
        [3] = { 0 },
    };

    rc = cbor_read_object(&cb->it, off_attr);
    if (rc || off == UINT_MAX) {
        return MGMT_ERR_EINVAL;
    }

    if (off == 0) {
        rc = img_upload_first(cb, img_data, data_len);
        if (rc != 0) {
            return rc;
        }
        img_ctxt.image_len = size;
    } else {
        if (!img_ctxt.uploading) {
            return MGMT_ERR_EINVAL;
        }

        if (off != img_ctxt.off) {
            /* Invalid offset. Drop the data, and respond with the offset we're
             * expecting data for.
             */
            return img_write_upload_rsp(cb, 0);
        }
    }

    if (data_len > 0) {
        last = img_ctxt.off + data_len == img_ctxt.image_len;
        rc = img_impl_write_image_data(off, img_data, data_len, last);
        if (rc != 0) {
            return rc;
        }

        img_ctxt.off += data_len;
        if (last) {
            img_ctxt.uploading = false;
        }
    }

    return img_write_upload_rsp(cb, 0);
}

int
img_group_register(void)
{
    return mgmt_group_register(&img_nmgr_group);
}
