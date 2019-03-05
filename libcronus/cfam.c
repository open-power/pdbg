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

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "buffer.h"
#include "instruction.h"
#include "libcronus_private.h"
#include "libcronus.h"

int cronus_getcfam(struct cronus_context *cctx,
		   int pib_index,
		   uint32_t addr,
		   uint32_t *value)
{
	struct cronus_buffer cbuf_request, cbuf_reply;
	struct cronus_reply reply;
	char devstr[4] = "0\0\0\0";
	uint32_t flags, key;
	uint32_t capacity, bits;
	int ret;

	assert(pib_index == 0 || pib_index == 1);
	devstr[0] = '1' + pib_index;

	ret = cbuf_new(&cbuf_request, 1024);
	if (ret)
		return ret;

	key = cronus_key(cctx);

	/* number of commands */
	cbuf_write_uint32(&cbuf_request, 1);

	/* header */
	cbuf_write_uint32(&cbuf_request, key);
	cbuf_write_uint32(&cbuf_request, INSTRUCTION_TYPE_FSI);
	cbuf_write_uint32(&cbuf_request, 7 * sizeof(uint32_t)); // payload size

	flags = INSTRUCTION_FLAG_DEVSTR | \
		INSTRUCTION_FLAG_CFAM_MAILBOX | \
		INSTRUCTION_FLAG_NO_PIB_RESET;

	/* payload */
	cbuf_write_uint32(&cbuf_request, 5);  // version
	cbuf_write_uint32(&cbuf_request, INSTRUCTION_CMD_READSPMEM);
	cbuf_write_uint32(&cbuf_request, flags);
	cbuf_write_uint32(&cbuf_request, addr);
	cbuf_write_uint32(&cbuf_request, 8 * sizeof(uint32_t));  // data size in bits
	cbuf_write_uint32(&cbuf_request, sizeof(devstr));
	cbuf_write(&cbuf_request, (uint8_t *)devstr, sizeof(devstr));

	ret = cronus_request(cctx, key, &cbuf_request, &cbuf_reply);
	if (ret) {
		fprintf(stderr, "Failed to talk to server\n");
		return ret;
	}

	ret = cronus_parse_reply(key, &cbuf_reply, &reply);
	if (ret) {
		fprintf(stderr, "Failed to parse reply\n");
		return ret;
	}

	cbuf_free(&cbuf_reply);

	if (reply.rc != SERVER_COMMAND_COMPLETE) {
		fprintf(stderr, "%s\n", reply.error);
		return EIO;
	}

	cbuf_init(&cbuf_reply, reply.data, reply.data_len);

	cbuf_read_uint32(&cbuf_reply, &capacity);
	if (capacity != 0x00000020) {
		fprintf(stderr, "Invalid capacity 0x%x\n", capacity);
		return EPROTO;
	}

	cbuf_read_uint32(&cbuf_reply, &bits);
	if (bits != 0x00000020) {
		fprintf(stderr, "Invalid number of bits 0x%x\n", bits);
		return EPROTO;
	}

	cbuf_read_uint32(&cbuf_reply, value);

	return 0;
}

int cronus_putcfam(struct cronus_context *cctx,
		   int pib_index,
		   uint32_t addr,
		   uint32_t value)
{
	struct cronus_buffer cbuf_request, cbuf_reply;
	struct cronus_reply reply;
	char devstr[4] = "0\0\0\0";
	uint32_t flags, key;
	int ret;

	assert(pib_index == 0 || pib_index == 1);
	devstr[0] = '1' + pib_index;

	ret = cbuf_new(&cbuf_request, 1024);
	if (ret)
		return ret;

	key = cronus_key(cctx);

	/* number of commands */
	cbuf_write_uint32(&cbuf_request, 1);

	/* header */
	cbuf_write_uint32(&cbuf_request, key);
	cbuf_write_uint32(&cbuf_request, INSTRUCTION_TYPE_FSI);
	cbuf_write_uint32(&cbuf_request, 11 * sizeof(uint32_t)); // payload size

	flags = INSTRUCTION_FLAG_DEVSTR | \
		INSTRUCTION_FLAG_CFAM_MAILBOX | \
		INSTRUCTION_FLAG_NO_PIB_RESET;

	/* payload */
	cbuf_write_uint32(&cbuf_request, 5);  // version
	cbuf_write_uint32(&cbuf_request, INSTRUCTION_CMD_WRITESPMEM);
	cbuf_write_uint32(&cbuf_request, flags);
	cbuf_write_uint32(&cbuf_request, addr);
	cbuf_write_uint32(&cbuf_request, 8 * sizeof(uint32_t));  // data size in bits
	cbuf_write_uint32(&cbuf_request, sizeof(devstr));
	cbuf_write_uint32(&cbuf_request, (1 + 1 + 1) * sizeof(uint32_t)); // size of value
	cbuf_write(&cbuf_request, (uint8_t *)devstr, sizeof(devstr));
	cbuf_write_uint32(&cbuf_request, 8 * sizeof(uint32_t)); // capacity in bits
	cbuf_write_uint32(&cbuf_request, 8 * sizeof(uint32_t)); // length in bits
	cbuf_write_uint32(&cbuf_request, value);

	ret = cronus_request(cctx, key, &cbuf_request, &cbuf_reply);
	if (ret) {
		fprintf(stderr, "Failed to talk to server\n");
		return ret;
	}

	ret = cronus_parse_reply(key, &cbuf_reply, &reply);
	if (ret) {
		fprintf(stderr, "Failed to parse reply\n");
		return ret;
	}

	cbuf_free(&cbuf_request);
	cbuf_free(&cbuf_reply);

	if (reply.rc != SERVER_COMMAND_COMPLETE) {
		fprintf(stderr, "%s\n", reply.error);
		return EIO;
	}

	return 0;
}

