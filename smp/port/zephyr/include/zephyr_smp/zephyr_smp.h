#ifndef H_ZEPHYR_SMP_
#define H_ZEPHYR_SMP_

struct zephyr_smp_transport;

/* XXX: Note: `zephyr_nmgr_pkt` is only used here during development.  Most
 * likely, a net_buf will be used instead.
 */
struct zephyr_nmgr_pkt;

/**
 * Transmit function.  The supplied mbuf is always consumed, regardless of
 * return code.
 */
typedef int zephyr_smp_transport_out_fn(struct zephyr_smp_transport *zst,
                                        struct zephyr_nmgr_pkt *pkt);
typedef uint16_t zephyr_smp_transport_get_mtu_fn(void);

struct zephyr_smp_transport {
    /* Must be the first member. */
    struct k_work zst_work;

    /* FIFO containing incoming requests to be processed. */
    struct k_fifo zst_fifo;

    zephyr_smp_transport_out_fn *zst_output;
    zephyr_smp_transport_get_mtu_fn *zst_get_mtu;
};

void zephyr_smp_transport_init(struct zephyr_smp_transport *zst,
                               zephyr_smp_transport_out_fn *output_func,
                               zephyr_smp_transport_get_mtu_fn *get_mtu_func);

int zephyr_smp_rx_req(struct zephyr_smp_transport *zst,
                      struct zephyr_nmgr_pkt *pkt);

#endif
