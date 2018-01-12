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

#include "sysinit/sysinit.h"
#include "os/os.h"
#include "mynewt_mgmt/mynewt_mgmt.h"
#include "smp/smp.h"
#include "mgmt_os/mgmt_os.h"
#include "mem/mem.h"
#include "cbor_mbuf_reader.h"
#include "cbor_mbuf_writer.h"
#include "mynewt_smp/mynewt_smp.h"

/* Shared queue that smp uses for work items. */
struct os_eventq *smp_evq;

static mgmt_alloc_rsp_fn mynewt_smp_alloc_rsp;
static mgmt_trim_front_fn mynewt_smp_trim_front;
static mgmt_reset_buf_fn mynewt_smp_reset_buf;
static mgmt_write_at_fn mynewt_smp_write_at;
static mgmt_init_reader_fn mynewt_smp_init_reader;
static mgmt_init_writer_fn mynewt_smp_init_writer;
static mgmt_free_buf_fn mynewt_smp_free_buf;
static smp_tx_rsp_fn mynewt_smp_tx_rsp;

static const struct mgmt_streamer_cfg mynewt_smp_cbor_cfg = {
    .alloc_rsp = mynewt_smp_alloc_rsp,
    .trim_front = mynewt_smp_trim_front,
    .reset_buf = mynewt_smp_reset_buf,
    .write_at = mynewt_smp_write_at,
    .init_reader = mynewt_smp_init_reader,
    .init_writer = mynewt_smp_init_writer,
    .free_buf = mynewt_smp_free_buf,
};

/**
 * Allocates an mbuf to contain an outgoing response fragment.
 */
static struct os_mbuf *
mynewt_smp_rsp_frag_alloc(uint16_t frag_size, void *arg)
{
    struct os_mbuf *src_rsp;
    struct os_mbuf *frag;

    /* We need to duplicate the user header from the source response, as that
     * is where transport-specific information is stored.
     */
    src_rsp = arg;

    frag = os_msys_get_pkthdr(frag_size, OS_MBUF_USRHDR_LEN(src_rsp));
    if (frag != NULL) {
        /* Copy the user header from the response into the fragment mbuf. */
        memcpy(OS_MBUF_USRHDR(frag), OS_MBUF_USRHDR(src_rsp),
               OS_MBUF_USRHDR_LEN(src_rsp));
    }

    return frag;
}

static void *
mynewt_smp_alloc_rsp(const void *req, void *arg)
{
    const struct os_mbuf *om_req;
    struct os_mbuf *om_rsp;

    om_req = req;
    om_rsp = os_msys_get_pkthdr(512, OS_MBUF_USRHDR_LEN(om_req));
    if (om_rsp == NULL) {
        return NULL;
    }

    /* Copy the request user header into the response. */
    memcpy(OS_MBUF_USRHDR(om_rsp),
           OS_MBUF_USRHDR(om_req),
           OS_MBUF_USRHDR_LEN(om_req));

    return om_rsp;
}

static int
mynewt_smp_trim_front(void *buf, int len, void *arg)
{
    struct os_mbuf *om;

    om = buf;
    os_mbuf_adj(om, len);
    return 0;
}

static void
mynewt_smp_reset_buf(void *buf, void *arg)
{
    struct os_mbuf *om;

    om = buf;
    os_mbuf_adj(om, -OS_MBUF_PKTLEN(om));
}

static int
mynewt_smp_write_at(struct cbor_encoder_writer *writer, int offset,
                    const void *data, int len, void *arg)
{
    struct cbor_mbuf_writer *mw;
    int rc;

    mw = (struct cbor_mbuf_writer *)writer;
    rc = os_mbuf_copyinto(mw->m, offset, data, len);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    writer->bytes_written = OS_MBUF_PKTLEN(mw->m);

    return 0;
}

static int
mynewt_smp_init_reader(struct cbor_decoder_reader *reader, void *buf,
                       void *arg)
{
    struct cbor_mbuf_reader *mr;

    mr = (struct cbor_mbuf_reader *)reader;
    cbor_mbuf_reader_init(mr, buf, 0);

    return 0;
}

static int
mynewt_smp_init_writer(struct cbor_encoder_writer *writer, void *buf,
                       void *arg)
{
    struct cbor_mbuf_writer *mw;

    mw = (struct cbor_mbuf_writer *)writer;
    cbor_mbuf_writer_init(mw, buf);

    return 0;
}

static int
mynewt_smp_tx_rsp(struct smp_streamer *ss, void *rsp, void *arg)
{
    struct mynewt_smp_transport *mst;
    struct os_mbuf *om_rsp;
    struct os_mbuf *frag;
    uint16_t mtu;
    int rc;

    mst = arg;
    om_rsp = rsp;

    mtu = mst->mst_get_mtu(rsp);
    if (mtu == 0) {
        /* The transport cannot support a transmission right now. */
        return MGMT_ERR_EUNKNOWN;
    }

    while (om_rsp != NULL) {
        frag = mem_split_frag(&om_rsp, mtu, mynewt_smp_rsp_frag_alloc,
                              om_rsp);
        if (frag == NULL) {
            return MGMT_ERR_ENOMEM;
        }

        rc = mst->mst_output(mst, frag);
        if (rc != 0) {
            /* Output function already freed mbuf. */
            return MGMT_ERR_EUNKNOWN;
        }
    }

    return MGMT_ERR_EOK;
}

static void
mynewt_smp_free_buf(void *buf, void *arg)
{
    os_mbuf_free_chain(buf);
}

static void
mynewt_smp_process(struct mynewt_smp_transport *mst)
{
    struct cbor_mbuf_reader reader;
    struct cbor_mbuf_writer writer;
    struct smp_streamer streamer;
    struct os_mbuf *req;
    int rc;

    streamer = (struct smp_streamer) {
        .mgmt_stmr = {
            .cfg = &mynewt_smp_cbor_cfg,
            .reader = &reader.r,
            .writer = &writer.enc,
            .cb_arg = mst,
        },
        .tx_rsp_cb = mynewt_smp_tx_rsp,
    };

    while (1) {
        req = os_mqueue_get(&mst->mst_imq);
        if (req == NULL) {
            break;
        }

        rc = smp_process_request_packet(&streamer, req);
        if (rc != 0) {
            break;
        }
    }
}

static void
mynewt_smp_event_data_in(struct os_event *ev)
{
    mynewt_smp_process(ev->ev_arg);
}

int
mynewt_smp_transport_init(struct mynewt_smp_transport *mst,
                          mynewt_smp_transport_out_fn *output_func,
                          mynewt_smp_transport_get_mtu_fn *get_mtu_func)
{
    int rc;

    *mst = (struct mynewt_smp_transport) {
        .mst_output = output_func,
        .mst_get_mtu = get_mtu_func,
    };

    rc = os_mqueue_init(&mst->mst_imq, mynewt_smp_event_data_in, mst);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
mynewt_smp_rx_req(struct mynewt_smp_transport *mst, struct os_mbuf *req)
{
    int rc;

    rc = os_mqueue_put(&mst->mst_imq, mgmt_evq_get(), req);
    if (rc != 0) {
        os_mbuf_free_chain(req);
    }

    return rc;
}

struct os_eventq *
mgmt_evq_get(void)
{
    return smp_evq;
}

void
mgmt_evq_set(struct os_eventq *evq)
{
    smp_evq = evq;
}

void
smp_pkg_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    rc = mgmt_os_group_register();
    SYSINIT_PANIC_ASSERT(rc == 0);

    mgmt_evq_set(os_eventq_dflt_get());
}
