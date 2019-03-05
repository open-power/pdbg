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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "buffer.h"
#include "instruction.h"
#include "libcronus_private.h"

int cronus_request(struct cronus_context *cctx,
		   uint32_t key,
		   struct cronus_buffer *request,
		   struct cronus_buffer *reply)
{
	uint8_t buf[1024], *ptr;
	size_t len = 0;
	ssize_t n;
	int ret;

	assert(cctx);
	assert(cctx->fd != -1);

	ptr = cbuf_finish(request, &len);
	assert(len > 0);

	n = write(cctx->fd, ptr, len);
	if (n == -1) {
		ret = errno;
		perror("write");
		return ret;
	}
	if (n != len) {
		fprintf(stderr, "Short write (%zu of %zu) to server\n", n, len);
		return EIO;
	}

	n = read(cctx->fd, buf, sizeof(buf));
	if (n == -1) {
		ret = errno;
		perror("read");
		return ret;
	}

	ret = cbuf_new_from_buf(reply, buf, n);
	if (ret)
		return ret;

	return 0;
}

static int cronus_parse_ecmd_dbuf(struct cronus_buffer *cbuf,
				  uint32_t size,
				  struct cronus_reply *reply)
{
	reply->data = malloc(size);
	if (!reply->data)
		return ENOMEM;
	reply->data_len = size;

	cbuf_read(cbuf, reply->data, reply->data_len);
	return 0;

}

static int cronus_parse_instruction_status(struct cronus_buffer *cbuf,
					   uint32_t size,
					   struct cronus_reply *reply)
{
	uint32_t offset;

	if (size < 4 * sizeof(uint32_t))
		return EPROTO;

	cbuf_read_uint32(cbuf, &reply->status_version);
	cbuf_read_uint32(cbuf, &reply->instruction_version);
	cbuf_read_uint32(cbuf, &reply->rc);
	cbuf_read_uint32(cbuf, &reply->status_len);
	offset = 4 * sizeof(uint32_t);

	if (size < offset + reply->status_len)
		return EPROTO;

	reply->status = malloc(reply->status_len);
	if (!reply->status)
		return ENOMEM;

	cbuf_read(cbuf, reply->status, reply->status_len);
	return 0;
}

int cronus_parse_reply(uint32_t key,
		       struct cronus_buffer *cbuf,
		       struct cronus_reply *reply)
{
	uint32_t num_replies;
	int i;

	memset(reply, 0, sizeof(*reply));

	cbuf_read_uint32(cbuf, &num_replies);
	if (num_replies != 2) {
		fprintf(stderr, "Invalid number of replies (%u) from server\n", num_replies);
		return EPROTO;
	}

	for (i=0; i<num_replies; i++) {
		uint32_t rkey, type, size;
		int ret = -1;

		cbuf_read_uint32(cbuf, &rkey);
		if (rkey != key) {
			fprintf(stderr, "Wrong key %u, expected %u\n", rkey, key);
			return EPROTO;
		}

		cbuf_read_uint32(cbuf, &type);
		cbuf_read_uint32(cbuf, &size);

		if (type == RESULT_TYPE_ECMD_DBUF) {
			ret = cronus_parse_ecmd_dbuf(cbuf, size, reply);
		} else if (type == RESULT_TYPE_INSTRUCTION_STATUS) {
			ret = cronus_parse_instruction_status(cbuf, size, reply);
		}

		if (ret != 0)
			return ret;
	}

	if (reply->rc != SERVER_COMMAND_COMPLETE) {
		uint32_t size;

		size = cbuf_size(cbuf) - cbuf_offset(cbuf);

		reply->error = malloc(size);
		if (!reply->error)
			return ENOMEM;

		cbuf_read(cbuf, (uint8_t *)reply->error, size);
	}

	return 0;
}
