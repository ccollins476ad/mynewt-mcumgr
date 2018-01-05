#include <assert.h>
#include <flash.h>
#include <zephyr.h>
#include <soc.h>
#include <init.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include "mgmt/mgmt.h"
#include "img_mgmt/img_mgmt_impl.h"
#include "img_mgmt/img_mgmt.h"

static struct device *zephyr_img_flash_dev;
static struct flash_img_context zephyr_img_flash_ctxt;

static int
img_mgmt_impl_flash_check_empty(off_t offset, size_t size, bool *out_empty)
{
    uint32_t data[16];
    off_t addr;
    off_t end;
    int bytes_to_read;
    int rc;
    int i;

    assert(size % 4 == 0);

    end = offset + size;
    for (addr = offset; addr < end; addr += sizeof data) {
        if (end - addr < sizeof data) {
            bytes_to_read = end - addr;
        } else {
            bytes_to_read = sizeof data;
        }

        rc = flash_read(zephyr_img_flash_dev, addr, data, bytes_to_read);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }

        for (i = 0; i < bytes_to_read / 4; i++) {
            if (data[i] != 0xffffffff) {
                *out_empty = false;
                return 0;
            }
        }
    }

    *out_empty = true;
    return 0;
}

static off_t
img_mgmt_impl_abs_offset(int slot, off_t sub_offset)
{
    off_t slot_start;

    switch (slot) {
    case 0:
        slot_start = FLASH_AREA_IMAGE_0_OFFSET;
        break;

    case 1:
        slot_start = FLASH_AREA_IMAGE_1_OFFSET;
        break;

    default:
        assert(0);
        slot_start = FLASH_AREA_IMAGE_1_OFFSET;
        break;
    }

    return slot_start + sub_offset;
}

int
img_mgmt_impl_erase_slot(void)
{
    bool empty;
    int rc;

    rc = img_mgmt_impl_flash_check_empty(FLASH_AREA_IMAGE_1_OFFSET,
                                         FLASH_AREA_IMAGE_1_SIZE,
                                         &empty);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    if (!empty) {
        rc = boot_erase_img_bank(FLASH_AREA_IMAGE_1_OFFSET);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }
    }

    return 0;
}

int
img_mgmt_impl_write_pending(int slot, bool permanent)
{
    int rc;

    if (slot != 1) {
        return MGMT_ERR_EINVAL;
    }

    rc = boot_request_upgrade(permanent);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
img_mgmt_impl_write_confirmed(void)
{
    int rc;

    rc = boot_write_img_confirmed();
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
img_mgmt_impl_read(int slot, unsigned int offset, void *dst,
                   unsigned int num_bytes)
{
    off_t abs_offset;
    int rc;

    abs_offset = img_mgmt_impl_abs_offset(slot, offset);
    rc = flash_read(zephyr_img_flash_dev, abs_offset, dst, num_bytes);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
img_mgmt_impl_write_image_data(unsigned int offset, const void *data,
                               unsigned int num_bytes, bool last)
{
    int rc;

    if (offset == 0) {
        flash_img_init(&zephyr_img_flash_ctxt, zephyr_img_flash_dev);
    }

    /* Cast away const. */
    rc = flash_img_buffered_write(&zephyr_img_flash_ctxt, (void *)data,
                                  num_bytes, false);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    if (last) {
        rc = flash_img_buffered_write(&zephyr_img_flash_ctxt, NULL, 0, true);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }
    }

    return 0;
}

int
img_mgmt_impl_swap_type(void)
{
    switch (boot_swap_type()) {
    case BOOT_SWAP_TYPE_NONE:
        return IMG_MGMT_SWAP_TYPE_NONE;
    case BOOT_SWAP_TYPE_TEST:
        return IMG_MGMT_SWAP_TYPE_TEST;
    case BOOT_SWAP_TYPE_PERM:
        return IMG_MGMT_SWAP_TYPE_PERM;
    case BOOT_SWAP_TYPE_REVERT:
        return IMG_MGMT_SWAP_TYPE_REVERT;
    default:
        assert(0);
        return IMG_MGMT_SWAP_TYPE_NONE;
    }
}

static int
img_mgmt_impl_init(struct device *dev)
{
    ARG_UNUSED(dev);

    zephyr_img_flash_dev = device_get_binding(FLASH_DRIVER_NAME);
    if (!zephyr_img_flash_dev) {
        return -ENODEV;
    }
    return 0;
}

SYS_INIT(img_mgmt_impl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);