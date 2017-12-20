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

/** SMP - Simple Management Protocol. */

#ifndef H_SMP_
#define H_SMP_

#include "mgmt/mgmt.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mynewt_smp_transport;
struct smp_streamer;
struct mgmt_hdr;

typedef int smp_tx_rsp_fn(struct smp_streamer *ss, void *buf, void *arg);

/** Decodes, encodes, and transmits SMP packets. */
struct smp_streamer {
    struct mgmt_streamer ss_base;
    smp_tx_rsp_fn *ss_tx_rsp;
};

void smp_ntoh_hdr(struct mgmt_hdr *hdr);
int smp_handle_single_payload(struct mgmt_cbuf *cbuf,
                              const struct mgmt_hdr *req_hdr);
int smp_process_single_packet(struct smp_streamer *streamer, void *req);

#ifdef __cplusplus
}
#endif

#endif /* H_SMP_ */
