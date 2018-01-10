#include <zephyr.h>
#include "net/buf.h"
#include "mgmt/mgmt.h"
#include "mgmt/buf.h"
#include "smp/smp.h"
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
    const struct net_buf_pool *pool;
    const struct net_buf *req_nb;
    struct net_buf *rsp_nb;

    req_nb = req;

    rsp_nb = mcumgr_buf_alloc();
    if (rsp_nb == NULL) {
        assert(0);
        return NULL;
    }

    pool = net_buf_pool_get(req_nb->pool_id);
    memcpy(net_buf_user_data(rsp_nb),
           net_buf_user_data((void *)req_nb),
           pool->user_data_size);

    return rsp_nb;
}

static int
zephyr_smp_trim_front(void *buf, int len, void *arg)
{
    struct net_buf *nb;

    nb = buf;
    if (len > nb->len) {
        len = nb->len;
    }

    net_buf_pull(nb, len);

    return 0;
}

static struct net_buf *
zephyr_smp_split_frag(struct net_buf **nb, uint16_t mtu)
{
    struct net_buf *frag;
    struct net_buf *src;

    src = *nb;

    if (src->len <= mtu) {
        *nb = NULL;
        frag = src;
    } else {
        frag = zephyr_smp_alloc_rsp(src, NULL);
        net_buf_add_mem(frag, src->data, mtu);

        zephyr_smp_trim_front(src, mtu, NULL);
    }

    return frag;
}

static void
zephyr_smp_reset_buf(void *buf, void *arg)
{
    net_buf_reset(buf);
}

static int
zephyr_smp_write_at(struct cbor_encoder_writer *writer, int offset,
                     const void *data, int len, void *arg)
{
    struct cbor_nb_writer *czw;
    struct net_buf *nb;

    czw = (struct cbor_nb_writer *)writer;
    nb = czw->nb;

    if (offset < 0 || offset > nb->len) {
        return MGMT_ERR_EINVAL;
    }

    if (len > net_buf_tailroom(nb)) {
        return MGMT_ERR_EINVAL;
    }

    memcpy(nb->data + offset, data, len);
    if (nb->len < offset + len) {
        nb->len = offset + len;
        writer->bytes_written = nb->len;
    }

    return 0;
}

static int
zephyr_smp_tx_rsp(struct smp_streamer *ns, void *rsp, void *arg)
{
    struct zephyr_smp_transport *zst;
    struct net_buf *frag;
    struct net_buf *nb;
    uint16_t mtu;
    int rc;
    int i;

    zst = arg;
    nb = rsp;

    mtu = zst->zst_get_mtu(rsp);
    if (mtu == 0) {
        /* The transport cannot support a transmission right now. */
        return MGMT_ERR_EUNKNOWN;
    }

    i = 0;
    while (nb != NULL) {
        frag = zephyr_smp_split_frag(&nb, mtu);
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
    mcumgr_buf_free(buf);
}

static int
zephyr_smp_init_reader(struct cbor_decoder_reader *reader, void *buf,
                        void *arg)
{
    struct cbor_nb_reader *czr;

    czr = (struct cbor_nb_reader *)reader;
    cbor_nb_reader_init(czr, buf, 0);

    return 0;
}

static int
zephyr_smp_init_writer(struct cbor_encoder_writer *writer, void *buf,
                        void *arg)
{
    struct cbor_nb_writer *czw;

    czw = (struct cbor_nb_writer *)writer;
    cbor_nb_writer_init(czw, buf);

    return 0;
}

static int
zephyr_smp_process_packet(struct zephyr_smp_transport *zst,
                          struct net_buf *nb)
{
    struct cbor_nb_reader reader;
    struct cbor_nb_writer writer;
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

    rc = smp_process_single_packet(&streamer, nb);
    return rc;
}

static void
zephyr_smp_handle_reqs(struct k_work *work)
{
    struct zephyr_smp_transport *zst;
    struct net_buf *nb;

    zst = (void *)work;

    while ((nb = k_fifo_get(&zst->zst_fifo, K_NO_WAIT)) != NULL) {
        zephyr_smp_process_packet(zst, nb);
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

int
zephyr_smp_rx_req(struct zephyr_smp_transport *zst,
                  struct net_buf *nb)
{
    k_fifo_put(&zst->zst_fifo, nb);
    k_work_submit(&zst->zst_work);

    return 0;
}
