#include <zephyr.h>
#include "mgmt/mgmt.h"
#include "smp/smp.h"
#include "znp/znp.h"
#include "zephyr_smp/zephyr_smp.h"

static mgmt_alloc_rsp_fn zephyr_smp_alloc_rsp;
static mgmt_trim_front_fn zephyr_smp_trim_front;
static mgmt_reset_buf_fn zephyr_smp_reset_buf;
static mgmt_write_at_fn zephyr_smp_write_at;
static mgmt_init_reader_fn zephyr_smp_init_reader;
static mgmt_init_writer_fn zephyr_smp_init_writer;
static mgmt_free_buf_fn zephyr_smp_free_buf;
static smp_tx_rsp_fn zephyr_smp_tx_rsp;

static const struct mgmt_streamer_cfg zephyr_smp_cbor_cfg = {
    .alloc_rsp = zephyr_smp_alloc_rsp,
    .trim_front = zephyr_smp_trim_front,
    .reset_buf = zephyr_smp_reset_buf,
    .write_at = zephyr_smp_write_at,
    .init_reader = zephyr_smp_init_reader,
    .init_writer = zephyr_smp_init_writer,
    .free_buf = zephyr_smp_free_buf,
};

static void *
zephyr_smp_alloc_rsp(const void *req, void *arg)
{
    const struct zephyr_nmgr_pkt *req_pkt;
    struct zephyr_nmgr_pkt *rsp_pkt;

    req_pkt = req;

    rsp_pkt = k_malloc(sizeof *rsp_pkt);
    if (rsp_pkt == NULL) {
        assert(0);
        return NULL;
    }
    rsp_pkt->len = 0;
    rsp_pkt->extra = req_pkt->extra;

    return rsp_pkt;
}

static struct zephyr_nmgr_pkt *
zephyr_smp_split_frag(struct zephyr_nmgr_pkt **pkt, uint16_t mtu)
{
    struct zephyr_nmgr_pkt *frag;
    struct zephyr_nmgr_pkt *src;

    src = *pkt;

    if (src->len <= mtu) {
        *pkt = NULL;
        frag = src;
    } else {
        frag = zephyr_smp_alloc_rsp(src, NULL);
        frag->len = mtu;
        memcpy(frag->data, src->data, mtu);

        src->len -= mtu;
        memmove(src->data, src->data + mtu, src->len);
    }

    return frag;
}

static int
zephyr_smp_trim_front(void *buf, int len, void *arg)
{
    struct zephyr_nmgr_pkt *pkt;

    if (len > 0) {
        pkt = buf;
        if (len >= pkt->len) {
            pkt->len = 0;
        } else {
            memmove(pkt->data, pkt->data + len, pkt->len - len);
            pkt->len -= len;
        }
    }

    return 0;
}

static void
zephyr_smp_reset_buf(void *buf, void *arg)
{
    struct zephyr_nmgr_pkt *pkt;

    pkt = buf;
    pkt->len = 0;
}

static int
zephyr_smp_write_at(struct cbor_encoder_writer *writer, int offset,
                     const void *data, int len, void *arg)
{
    struct cbor_znp_writer *czw;
    struct zephyr_nmgr_pkt *pkt;

    czw = (struct cbor_znp_writer *)writer;
    pkt = czw->pkt;

    if (offset < 0 || offset > pkt->len) {
        return MGMT_ERR_EINVAL;
    }

    if (offset + len > sizeof pkt->data) {
        return MGMT_ERR_EINVAL;
    }

    memcpy(pkt->data + offset, data, len);
    if (pkt->len < offset + len) {
        pkt->len = offset + len;
        writer->bytes_written = pkt->len;
    }

    return 0;
}

static int
zephyr_smp_tx_rsp(struct smp_streamer *ns, void *rsp, void *arg)
{
    struct zephyr_smp_transport *zst;
    struct zephyr_nmgr_pkt *frag;
    struct zephyr_nmgr_pkt *pkt;
    uint16_t mtu;
    int rc;
    int i;

    zst = arg;
    pkt = rsp;

    mtu = zst->zst_get_mtu(rsp);
    if (mtu == 0) {
        /* The transport cannot support a transmission right now. */
        return MGMT_ERR_EUNKNOWN;
    }

    i = 0;
    while (pkt != NULL) {
        frag = zephyr_smp_split_frag(&pkt, mtu);
        if (frag == NULL) {
            return MGMT_ERR_ENOMEM;
        }

        rc = zst->zst_output(zst, frag);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }
    }

    return 0;
}

static void
zephyr_smp_free_buf(void *buf, void *arg)
{
    k_free(buf);
}

static int
zephyr_smp_init_reader(struct cbor_decoder_reader *reader, void *buf,
                        void *arg)
{
    struct cbor_znp_reader *czr;

    czr = (struct cbor_znp_reader *)reader;
    cbor_znp_reader_init(czr, buf, 0);

    return 0;
}

static int
zephyr_smp_init_writer(struct cbor_encoder_writer *writer, void *buf,
                        void *arg)
{
    struct cbor_znp_writer *czw;

    czw = (struct cbor_znp_writer *)writer;
    cbor_znp_writer_init(czw, buf);

    return 0;
}

static int
zephyr_smp_process_packet(struct zephyr_smp_transport *zst,
                          struct zephyr_nmgr_pkt *pkt)
{
    struct cbor_znp_reader reader;
    struct cbor_znp_writer writer;
    struct smp_streamer streamer;
    int rc;

    streamer = (struct smp_streamer) {
        .ss_base = {
            .cfg = &zephyr_smp_cbor_cfg,
            .reader = &reader.r,
            .writer = &writer.enc,
            .cb_arg = zst,
        },
        .ss_tx_rsp = zephyr_smp_tx_rsp,
    };

    rc = smp_process_single_packet(&streamer, pkt);
    return rc;
}

static void
zephyr_smp_handle_reqs(struct k_work *work)
{
    struct zephyr_smp_transport *zst;
    struct zephyr_nmgr_pkt *pkt;

    zst = (void *)work;

    while ((pkt = k_fifo_get(&zst->zst_fifo, K_NO_WAIT)) != NULL) {
        zephyr_smp_process_packet(zst, pkt);
    }
}

void
zephyr_smp_transport_init(struct zephyr_smp_transport *zst,
                          zephyr_smp_transport_out_fn *output_func,
                          zephyr_smp_transport_get_mtu_fn *get_mtu_func)
{
    *zst = (struct zephyr_smp_transport) {
        .zst_output = output_func,
        .zst_get_mtu = get_mtu_func,
    };

    k_work_init(&zst->zst_work, zephyr_smp_handle_reqs);
    k_fifo_init(&zst->zst_fifo);
}

/* XXX: Note: `zephyr_nmgr_pkt` is only used here during development.  Most
 * likely, a net_buf will be used instead.
 */
int
zephyr_smp_rx_req(struct zephyr_smp_transport *zst,
                  struct zephyr_nmgr_pkt *pkt)
{
    k_fifo_put(&zst->zst_fifo, pkt);
    k_work_submit(&zst->zst_work);

    return 0;
}
