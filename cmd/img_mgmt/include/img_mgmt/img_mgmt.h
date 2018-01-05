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

#ifndef H_IMG_MGMT_
#define H_IMG_MGMT_

#include <inttypes.h>
struct image_version;

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_MGMT_ID_STATE           0
#define IMG_MGMT_ID_UPLOAD          1
#define IMG_MGMT_ID_FILE            2
#define IMG_MGMT_ID_CORELIST        3
#define IMG_MGMT_ID_CORELOAD        4
#define IMG_MGMT_ID_ERASE           5

#define IMG_MGMT_MAX_NAME           64
#define IMG_MGMT_MAX_VER            25  /* 255.255.65535.4294967295\0 */

#define IMG_MGMT_STATE_F_PENDING    0x01
#define IMG_MGMT_STATE_F_CONFIRMED  0x02
#define IMG_MGMT_STATE_F_ACTIVE     0x04
#define IMG_MGMT_STATE_F_PERMANENT  0x08

#define IMG_MGMT_SWAP_TYPE_NONE     0
#define IMG_MGMT_SWAP_TYPE_TEST     1
#define IMG_MGMT_SWAP_TYPE_PERM     2
#define IMG_MGMT_SWAP_TYPE_REVERT   3

/**
 * Take version and convert it to string in dst.
 */
int img_mgmt_ver_str(const struct image_version *ver, char *dst);

/**
 * Given flash_map slot id, read in image_version and/or image hash.
 */
int img_mgmt_read_info(int image_slot, struct image_version *ver,
                       uint8_t *hash, uint32_t *flags);

uint8_t img_mgmt_state_flags(int query_slot);
int img_mgmt_slot_in_use(int slot);
int img_mgmt_group_register(void);

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_MGMT_ */
