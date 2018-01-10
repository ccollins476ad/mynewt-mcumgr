#ifndef H_ZEPHYR_SMP_
#define H_ZEPHYR_SMP_

struct zephyr_smp_transport;
struct net_buf;

/**
 * Transmit function.  The supplied mbuf is always consumed, regardless of
 * return code.
 */
typedef int zephyr_smp_transport_out_fn(struct zephyr_smp_transport *zst,
                                        struct net_buf *nb);

/**
 * MTU query function.  The supplied packet should contain a request received
 * from the peer whose MTU is being queried.  This function takes a packet
 * parameter because some transports store connection-specific information in
 * the packet (e.g., the BLE transport stores the peer address).
 *
 * @return                      The transport's MTU;
 *                              0 if transmission is currently not possible.
 */
typedef uint16_t
zephyr_smp_transport_get_mtu_fn(const struct net_buf *nb);

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
                      struct net_buf *nb);

#endif
