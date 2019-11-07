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
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#include "buffer.h"
#include "instruction.h"
#include "libcronus_private.h"
#include "libcronus.h"

int cronus_submit(struct cronus_context *cctx,
		  int pib_index,
		  uint8_t *sbefifo_request,
		  uint32_t request_len,
		  uint8_t *sbefifo_reply,
		  uint32_t *reply_len)
{
	struct cronus_buffer cbuf_request, cbuf_reply;
	struct cronus_reply reply;
	char devstr[4] = "0\0\0\0";
	uint32_t flags, key, timeout;
	uint32_t capacity, bits;
	int ret;

	assert(pib_index == 0 || pib_index == 1);
	devstr[0] = '1' + pib_index;

	ret = cbuf_new(&cbuf_request, 1024 + request_len);
	if (ret)
		return ret;

	key = cronus_key(cctx);

	/* number of commands */
	cbuf_write_uint32(&cbuf_request, 1);

	/* header */
	cbuf_write_uint32(&cbuf_request, key);
	cbuf_write_uint32(&cbuf_request, INSTRUCTION_TYPE_SBEFIFO);
	cbuf_write_uint32(&cbuf_request, (10 + request_len/4) * sizeof(uint32_t)); // payload size

	flags = INSTRUCTION_FLAG_64BIT_ADDRESS | \
		INSTRUCTION_FLAG_DEVSTR | \
		INSTRUCTION_FLAG_NO_PIB_RESET;

	timeout = 30;

	/* payload */
	cbuf_write_uint32(&cbuf_request, 1);  // version
	cbuf_write_uint32(&cbuf_request, INSTRUCTION_CMD_SUBMIT);
	cbuf_write_uint32(&cbuf_request, flags);
	cbuf_write_uint32(&cbuf_request, timeout);
	cbuf_write_uint32(&cbuf_request, *reply_len);
	cbuf_write_uint32(&cbuf_request, sizeof(devstr));
	cbuf_write_uint32(&cbuf_request, request_len);
	cbuf_write(&cbuf_request, (uint8_t *)devstr, sizeof(devstr));
	cbuf_write_uint32(&cbuf_request, request_len*8);
	cbuf_write_uint32(&cbuf_request, request_len*8);
	cbuf_write(&cbuf_request, sbefifo_request, request_len);

	ret = cronus_request(cctx, key, *reply_len, &cbuf_request, &cbuf_reply);
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

	cbuf_init(&cbuf_reply, reply.data, reply.data_len);

	cbuf_read_uint32(&cbuf_reply, &capacity);
	cbuf_read_uint32(&cbuf_reply, &bits);

	*reply_len = capacity / 8;
	cbuf_read(&cbuf_reply, sbefifo_reply, *reply_len);

	return 0;
}
