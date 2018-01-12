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

#ifndef H_MYNEWT_SMP_
#define H_MYNEWT_SMP_

#include <inttypes.h>
#include "mgmt/mgmt.h"
#include "os/os_mbuf.h"
struct mynewt_smp_transport;

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef mynewt_smp_transport_out_fn
 * @brief SMP transmit function for Mynewt.
 *
 * The supplied mbuf is always consumed, regardless of return code.
 *
 * @param mst                   The transport to send via.
 * @param om                    The mbuf to transmit.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
typedef int mynewt_smp_transport_out_fn(struct mynewt_smp_transport *mst,
                                        struct os_mbuf *om);

/** @typedef mynewt_smp_transport_get_mtu_fn
 * @brief SMP MTU query function for Mynewt.
 *
 * The supplied mbuf should contain a request received from the peer whose MTU
 * is being queried.  This function takes an mbuf parameter because some
 * transports store connection-specific information in the mbuf user header
 * (e.g., the BLE transport stores the connection handle).
 *
 * @param om                    Contains a request from the relevant peer.
 *
 * @return                      The transport's MTU;
 *                              0 if transmission is currently not possible.
 */
typedef uint16_t mynewt_smp_transport_get_mtu_fn(struct os_mbuf *om);

/**
 * @brief Provides Mynewt-specific functionality for sending SMP responses.
 */ 
struct mynewt_smp_transport {
    struct os_mqueue mst_imq;
    mynewt_smp_transport_out_fn *mst_output;
    mynewt_smp_transport_get_mtu_fn *mst_get_mtu;
};

/**
 * @brief Initializes a Mynewt SMP transport object.
 *
 * @param mst                   The transport to construct.
 * @param output_func           The transport's send function.
 * @param get_mtu_func          The transport's get-MTU function.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int mynewt_smp_transport_init(struct mynewt_smp_transport *mst,
                              mynewt_smp_transport_out_fn *output_func,
                              mynewt_smp_transport_get_mtu_fn *get_mtu_func);

/**
 * @brief Processes an incoming SMP request packet.
 *
 * @param mst                   The transport to use to send the corresponding
 *                                  response(s).
 * @param req                   The request packet to process.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int mynewt_smp_rx_req(struct mynewt_smp_transport *mst, struct os_mbuf *req);

#ifdef __cplusplus
}
#endif

#endif /* H_MYNEWT_SMP_ */
