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
#include "net/buf.h"
#include "zephyr_mgmt/buf.h"
#include "compilersupport_p.h"

NET_BUF_POOL_DEFINE(pkt_pool, CONFIG_MCUMGR_BUF_COUNT, CONFIG_MCUMGR_BUF_SIZE,
                    CONFIG_MCUMGR_BUF_USER_DATA_SIZE, NULL);

struct net_buf *
mcumgr_buf_alloc(void)
{
    return net_buf_alloc(&pkt_pool, K_NO_WAIT);
}

void
mcumgr_buf_free(struct net_buf *nb)
{
    net_buf_unref(nb);
}

static int
cbor_nb_read(struct mgmt_reader *reader, void *dst, size_t offset, size_t len)
{
    struct cbor_nb_reader *cnr;

    cnr = (struct cbor_nb_reader *) reader;

    if (offset < 0 || offset > cnr->nb->len - (int)len) {
        return -1;
    }

    memcpy(dst, cnr->nb->data + offset, len);
    return 0;
}

void
cbor_nb_reader_init(struct cbor_nb_reader *cnr,
                    struct net_buf *nb)
{
    cnr->nb = nb;
    cnr->r.read_cb = cbor_nb_read;
    cnr->r.message_size = nb->len;
}

static int
cbor_nb_write(struct mgmt_writer *writer, const void *data, size_t len)
{
    struct cbor_nb_writer *cnw;

    cnw = (struct cbor_nb_writer *) writer;
    if (len > net_buf_tailroom(cnw->nb)) {
        return CborErrorOutOfMemory;
    }

    net_buf_add_mem(cnw->nb, data, len);

    return CborNoError;
}

void
cbor_nb_writer_init(struct cbor_nb_writer *cnw, struct net_buf *nb)
{
    cnw->nb = nb;
    cnw->w.write_cb = &cbor_nb_write;
}
