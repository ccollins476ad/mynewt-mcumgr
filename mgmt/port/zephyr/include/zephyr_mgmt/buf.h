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

#ifndef H_MGMT_BUF
#define H_MGMT_BUF

#include <inttypes.h>
#include "cbor_encoder_writer.h"
#include "cbor_decoder_reader.h"
struct net_buf;

struct cbor_nb_reader {
    struct cbor_decoder_reader r;
    struct net_buf *nb;
};

struct cbor_nb_writer {
    struct cbor_encoder_writer enc;
    struct net_buf *nb;
};

/**
 * @brief Allocates a net_buf for holding an mcumgr request or response.
 *
 * @return                      A newly-allocated buffer net_buf on success;
 *                              NULL on failure.
 */
struct net_buf *mcumgr_buf_alloc(void);

/**
 * @brief Frees an mcumgr net_buf
 *
 * @param nb                    The net_buf to free.
 */
void mcumgr_buf_free(struct net_buf *nb);

/**
 * @brief Initializes a CBOR writer with the specified net_buf.
 *
 * @param cnw                   The writer to initialize.
 * @param nb                    The net_buf that the writer will write to.
 */
void cbor_nb_writer_init(struct cbor_nb_writer *cnw,
                         struct net_buf *nb);

/**
 * @brief Initializes a CBOR reader with the specified net_buf.
 *
 * @param cnr                   The reader to initialize.
 * @param nb                    The net_buf that the reader will read from.
 */
void cbor_nb_reader_init(struct cbor_nb_reader *cnr,
                         struct net_buf *nb);

#endif
