/* Copyright 2019 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LIBCRONUS_PRIVATE_H__
#define __LIBCRONUS_PRIVATE_H__

#include <stdint.h>

#include "buffer.h"

struct cronus_context {
	int fd;
	uint32_t key;

	uint32_t server_version;
};

struct cronus_reply {
	uint32_t status_version;
	uint32_t instruction_version;
	uint32_t rc;
	uint32_t status_len;
	uint8_t *status;
	char *error;

	uint32_t data_len;
	uint8_t *data;
};

uint32_t cronus_key(struct cronus_context *cctx);

int cronus_request(struct cronus_context *cctx,
		   uint32_t key, uint32_t out_len,
		   struct cronus_buffer *request,
		   struct cronus_buffer *reply);
int cronus_parse_reply(uint32_t key,
		       struct cronus_buffer *cbuf,
		       struct cronus_reply *reply);

#endif /* __LIBCRONUS_PRIVATE_H__ */
