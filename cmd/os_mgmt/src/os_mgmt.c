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

#include <assert.h>
#include <string.h>
#include "mgmt/mgmt.h"
#include "os_mgmt/os_mgmt.h"
#include "os_mgmt/os_mgmt_impl.h"
#include "cbor.h"
#include "cborattr/cborattr.h"

static mgmt_handler_fn os_mgmt_echo;
static mgmt_handler_fn os_mgmt_reset;
static mgmt_handler_fn os_mgmt_taskstat_read;

#if 0
static int os_mgmt_console_echo(struct mgmt_cbuf *);
static int os_mgmt_taskstat_read(struct mgmt_cbuf *njb);
static int os_mgmt_mpstat_read(struct mgmt_cbuf *njb);
static int os_mgmt_datetime_get(struct mgmt_cbuf *njb);
static int os_mgmt_datetime_set(struct mgmt_cbuf *njb);
#endif

static const struct mgmt_handler os_mgmt_group_handlers[] = {
    [OS_MGMT_ID_ECHO] = {
        os_mgmt_echo, os_mgmt_echo
    },
#if 0
    [OS_MGMT_ID_CONS_ECHO_CTRL] = {
        os_mgmt_console_echo, os_mgmt_console_echo
    },
    [OS_MGMT_ID_MPSTAT] = {
        os_mgmt_mpstat_read, NULL
    },
    [OS_MGMT_ID_DATETIME_STR] = {
        os_mgmt_datetime_get, os_mgmt_datetime_set
    },
#endif
    [OS_MGMT_ID_TASKSTAT] = {
        os_mgmt_taskstat_read, NULL
    },
    [OS_MGMT_ID_RESET] = {
        NULL, os_mgmt_reset
    },
};

#define OS_MGMT_GROUP_SZ    \
    (sizeof os_mgmt_group_handlers / sizeof os_mgmt_group_handlers[0])

static struct mgmt_group os_mgmt_group = {
    .mg_handlers = os_mgmt_group_handlers,
    .mg_handlers_count = OS_MGMT_GROUP_SZ,
    .mg_group_id = MGMT_GROUP_ID_OS,
};

static int
os_mgmt_echo(struct mgmt_cbuf *cb)
{
    char echo_buf[128] = {'\0'};
    CborError g_err = CborNoError;

    const struct cbor_attr_t attrs[2] = {
        [0] = {
            .attribute = "d",
            .type = CborAttrTextStringType,
            .addr.string = echo_buf,
            .nodefault = 1,
            .len = sizeof(echo_buf),
        },
        [1] = {
            .attribute = NULL
        }
    };

    g_err |= cbor_encode_text_stringz(&cb->encoder, "r");
    g_err |= cbor_read_object(&cb->it, attrs);
    g_err |= cbor_encode_text_string(&cb->encoder, echo_buf, strlen(echo_buf));

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return (0);
}

#if 0
static int
os_mgmt_console_echo(struct mgmt_cbuf *cb)
{
    long long int echo_on = 1;
    int rc;
    const struct cbor_attr_t attrs[2] = {
        [0] = {
            .attribute = "echo",
            .type = CborAttrIntegerType,
            .addr.integer = &echo_on,
            .nodefault = 1
        },
        [1] = { 0 },
    };

    rc = cbor_read_object(&cb->it, attrs);
    if (rc) {
        return MGMT_ERR_EINVAL;
    }

    if (echo_on) {
        console_echo(1);
    } else {
        console_echo(0);
    }
    return (0);
}

static int
os_mgmt_taskstat_read(struct mgmt_cbuf *cb)
{
    struct os_task *prev_task;
    struct os_task_info oti;
    CborError g_err = CborNoError;
    CborEncoder tasks;
    CborEncoder task;

    g_err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);
    g_err |= cbor_encode_text_stringz(&cb->encoder, "tasks");
    g_err |= cbor_encoder_create_map(&cb->encoder, &tasks,
                                     CborIndefiniteLength);

    prev_task = NULL;
    while (1) {
        prev_task = os_task_info_get_next(prev_task, &oti);
        if (prev_task == NULL) {
            break;
        }

        g_err |= cbor_encode_text_stringz(&tasks, oti.oti_name);
        g_err |= cbor_encoder_create_map(&tasks, &task, CborIndefiniteLength);
        g_err |= cbor_encode_text_stringz(&task, "prio");
        g_err |= cbor_encode_uint(&task, oti.oti_prio);
        g_err |= cbor_encode_text_stringz(&task, "tid");
        g_err |= cbor_encode_uint(&task, oti.oti_taskid);
        g_err |= cbor_encode_text_stringz(&task, "state");
        g_err |= cbor_encode_uint(&task, oti.oti_state);
        g_err |= cbor_encode_text_stringz(&task, "stkuse");
        g_err |= cbor_encode_uint(&task, oti.oti_stkusage);
        g_err |= cbor_encode_text_stringz(&task, "stksiz");
        g_err |= cbor_encode_uint(&task, oti.oti_stksize);
        g_err |= cbor_encode_text_stringz(&task, "cswcnt");
        g_err |= cbor_encode_uint(&task, oti.oti_cswcnt);
        g_err |= cbor_encode_text_stringz(&task, "runtime");
        g_err |= cbor_encode_uint(&task, oti.oti_runtime);
        g_err |= cbor_encode_text_stringz(&task, "last_checkin");
        g_err |= cbor_encode_uint(&task, oti.oti_last_checkin);
        g_err |= cbor_encode_text_stringz(&task, "next_checkin");
        g_err |= cbor_encode_uint(&task, oti.oti_next_checkin);
        g_err |= cbor_encoder_close_container(&tasks, &task);
    }
    g_err |= cbor_encoder_close_container(&cb->encoder, &tasks);

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return (0);
}

static int
os_mgmt_mpstat_read(struct mgmt_cbuf *cb)
{
    struct os_mempool *prev_mp;
    struct os_mempool_info omi;
    CborError g_err = CborNoError;
    CborEncoder pools;
    CborEncoder pool;

    g_err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);
    g_err |= cbor_encode_text_stringz(&cb->encoder, "mpools");
    g_err |= cbor_encoder_create_map(&cb->encoder, &pools,
                                     CborIndefiniteLength);

    prev_mp = NULL;
    while (1) {
        prev_mp = os_mempool_info_get_next(prev_mp, &omi);
        if (prev_mp == NULL) {
            break;
        }

        g_err |= cbor_encode_text_stringz(&pools, omi.omi_name);
        g_err |= cbor_encoder_create_map(&pools, &pool, CborIndefiniteLength);
        g_err |= cbor_encode_text_stringz(&pool, "blksiz");
        g_err |= cbor_encode_uint(&pool, omi.omi_block_size);
        g_err |= cbor_encode_text_stringz(&pool, "nblks");
        g_err |= cbor_encode_uint(&pool, omi.omi_num_blocks);
        g_err |= cbor_encode_text_stringz(&pool, "nfree");
        g_err |= cbor_encode_uint(&pool, omi.omi_num_free);
        g_err |= cbor_encode_text_stringz(&pool, "min");
        g_err |= cbor_encode_uint(&pool, omi.omi_min_free);
        g_err |= cbor_encoder_close_container(&pools, &pool);
    }

    g_err |= cbor_encoder_close_container(&cb->encoder, &pools);

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return (0);
}

static int
os_mgmt_datetime_get(struct mgmt_cbuf *cb)
{
    struct os_timeval tv;
    struct os_timezone tz;
    char buf[DATETIME_BUFSIZE];
    int rc;
    CborError g_err = CborNoError;

    g_err |= cbor_encode_text_stringz(&cb->encoder, "rc");
    g_err |= cbor_encode_int(&cb->encoder, MGMT_ERR_EOK);

    /* Display the current datetime */
    rc = os_gettimeofday(&tv, &tz);
    assert(rc == 0);
    rc = datetime_format(&tv, &tz, buf, DATETIME_BUFSIZE);
    if (rc) {
        rc = MGMT_ERR_EINVAL;
        goto err;
    }
    g_err |= cbor_encode_text_stringz(&cb->encoder, "datetime");
    g_err |= cbor_encode_text_stringz(&cb->encoder, buf);

    if (g_err) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;

err:
    return (rc);
}

static int
os_mgmt_datetime_set(struct mgmt_cbuf *cb)
{
    struct os_timeval tv;
    struct os_timezone tz;
    char buf[DATETIME_BUFSIZE];
    int rc = 0;
    const struct cbor_attr_t datetime_write_attr[] = {
        [0] = {
            .attribute = "datetime",
            .type = CborAttrTextStringType,
            .addr.string = buf,
            .len = sizeof(buf),
        },
        { 0 },
    };

    rc = cbor_read_object(&cb->it, datetime_write_attr);
    if (rc) {
        return MGMT_ERR_EINVAL;
    }

    /* Set the current datetime */
    rc = datetime_parse(buf, &tv, &tz);
    if (!rc) {
        rc = os_settimeofday(&tv, &tz);
        if (rc) {
          return MGMT_ERR_EINVAL;
        }
    } else {
        return MGMT_ERR_EINVAL;
    }

    rc = mgmt_cbuf_setoerr(cb, 0);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static void
os_mgmt_reset_tmo(struct os_event *ev)
{
    /*
     * Tickle watchdog just before re-entering bootloader.
     * Depending on what system has been doing lately, watchdog
     * timer might be close to firing.
     */
    hal_watchdog_tickle();
    hal_system_reset();
}
#endif

static int
os_mgmt_taskstat_encode_one(struct CborEncoder *encoder,
                            const struct os_mgmt_task_info *task_info)
{
    CborEncoder task_map;
    CborError err;

    err = 0;
    err |= cbor_encode_text_stringz(encoder, task_info->oti_name);
    err |= cbor_encoder_create_map(encoder, &task_map, CborIndefiniteLength);
    err |= cbor_encode_text_stringz(&task_map, "prio");
    err |= cbor_encode_uint(&task_map, task_info->oti_prio);
    err |= cbor_encode_text_stringz(&task_map, "tid");
    err |= cbor_encode_uint(&task_map, task_info->oti_taskid);
    err |= cbor_encode_text_stringz(&task_map, "state");
    err |= cbor_encode_uint(&task_map, task_info->oti_state);
    err |= cbor_encode_text_stringz(&task_map, "stkuse");
    err |= cbor_encode_uint(&task_map, task_info->oti_stkusage);
    err |= cbor_encode_text_stringz(&task_map, "stksiz");
    err |= cbor_encode_uint(&task_map, task_info->oti_stksize);
    err |= cbor_encode_text_stringz(&task_map, "cswcnt");
    err |= cbor_encode_uint(&task_map, task_info->oti_cswcnt);
    err |= cbor_encode_text_stringz(&task_map, "runtime");
    err |= cbor_encode_uint(&task_map, task_info->oti_runtime);
    err |= cbor_encode_text_stringz(&task_map, "last_checkin");
    err |= cbor_encode_uint(&task_map, task_info->oti_last_checkin);
    err |= cbor_encode_text_stringz(&task_map, "next_checkin");
    err |= cbor_encode_uint(&task_map, task_info->oti_next_checkin);
    err |= cbor_encoder_close_container(encoder, &task_map);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

static int
os_mgmt_taskstat_read(struct mgmt_cbuf *cb)
{
    struct os_mgmt_task_info task_info;
    struct CborEncoder tasks_map;
    CborError err;
    int task_idx;
    int rc;

    err = 0;

    err |= cbor_encode_text_stringz(&cb->encoder, "tasks");
    err |= cbor_encoder_create_map(&cb->encoder, &tasks_map,
                                   CborIndefiniteLength);
    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    task_idx = 0;
    while (1) {
        rc = os_mgmt_impl_task_info(task_idx, &task_info);
        if (rc == MGMT_ERR_ENOENT) {
            break;
        } else if (rc != 0) {
            return rc;
        }

        rc = os_mgmt_taskstat_encode_one(&tasks_map, &task_info);
        if (rc != 0) {
            return rc;
        }

        task_idx++;
    }

    err = cbor_encoder_close_container(&cb->encoder, &tasks_map);
    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

static int
os_mgmt_reset(struct mgmt_cbuf *cb)
{
    return os_mgmt_impl_reset();
}

int
os_mgmt_group_register(void)
{
    return mgmt_group_register(&os_mgmt_group);
}
