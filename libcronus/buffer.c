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
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <endian.h>

#include "buffer.h"

int cbuf_new(struct cronus_buffer *cbuf, size_t size)
{
	cbuf->ptr = malloc(size);
	if (!cbuf->ptr)
		return ENOMEM;

	cbuf->size = size;
	cbuf->offset = 0;

	return 0;
}

int cbuf_new_from_buf(struct cronus_buffer *cbuf, uint8_t *ptr, size_t size)
{
	int ret;

	ret = cbuf_new(cbuf, size);
	if (ret)
		return ret;

	memcpy(cbuf->ptr, ptr, size);
	return 0;
}

void cbuf_free(struct cronus_buffer *cbuf)
{
	free(cbuf->ptr);
}

void cbuf_init(struct cronus_buffer *cbuf, uint8_t *ptr, size_t size)
{
	*cbuf = (struct cronus_buffer) {
		.ptr = ptr,
		.size = size,
	};
}

uint8_t *cbuf_ptr(struct cronus_buffer *cbuf)
{
	return cbuf->ptr + cbuf->offset;
}

size_t cbuf_offset(struct cronus_buffer *cbuf)
{
	return cbuf->offset;
}

size_t cbuf_size(struct cronus_buffer *cbuf)
{
	return cbuf->size;
}

uint8_t *cbuf_finish(struct cronus_buffer *cbuf, size_t *length)
{
	*length = cbuf->offset;
	return cbuf->ptr;
}

static void cbuf_check_offset(struct cronus_buffer *cbuf, size_t count)
{
	assert(cbuf->offset + count <= cbuf->size);
}

void cbuf_write(struct cronus_buffer *cbuf, uint8_t *bytes, size_t count)
{
	cbuf_check_offset(cbuf, count);

	memcpy(cbuf->ptr + cbuf->offset, bytes, count);
	cbuf->offset += count;
}

void cbuf_read(struct cronus_buffer *cbuf, uint8_t *bytes, size_t count)
{
	cbuf_check_offset(cbuf, count);

	memcpy(bytes, cbuf->ptr + cbuf->offset, count);
	cbuf->offset += count;
}

void cbuf_write_uint8(struct cronus_buffer *cbuf, uint8_t value)
{
	cbuf_write(cbuf, &value, 1);
}

void cbuf_read_uint8(struct cronus_buffer *cbuf, uint8_t *value)
{
	cbuf_read(cbuf, value, 1);
}

void cbuf_write_uint16(struct cronus_buffer *cbuf, uint16_t value)
{
	uint16_t data = htobe16(value);

	cbuf_write(cbuf, (uint8_t *)&data, 2);
}

void cbuf_read_uint16(struct cronus_buffer *cbuf, uint16_t *value)
{
	uint16_t data;

	cbuf_read(cbuf, (uint8_t *)&data, 2);
	*value = be16toh(data);
}

void cbuf_write_uint32(struct cronus_buffer *cbuf, uint32_t value)
{
	uint32_t data = htobe32(value);

	cbuf_write(cbuf, (uint8_t *)&data, 4);
}

void cbuf_read_uint32(struct cronus_buffer *cbuf, uint32_t *value)
{
	uint32_t data;

	cbuf_read(cbuf, (uint8_t *)&data, 4);
	*value = be32toh(data);
}

void cbuf_write_uint64(struct cronus_buffer *cbuf, uint64_t value)
{
	uint64_t data = htobe64(value);

	cbuf_write(cbuf, (uint8_t *)&data, 8);
}

void cbuf_read_uint64(struct cronus_buffer *cbuf, uint64_t *value)
{
	uint64_t data;

	cbuf_read(cbuf, (uint8_t *)&data, 8);
	*value = be64toh(data);
}

void cbuf_dump(struct cronus_buffer *cbuf, const char *prefix)
{
	size_t len;
	unsigned int i;

	if (!prefix)
		prefix = "DUMP";

	len = (cbuf->offset > 0) ? cbuf->offset : cbuf->size;

	for (i=0; i<len; i++) {
		if (i % 16 == 0)
			fprintf(stderr, "%s: 0x%08x", prefix, i);

		fprintf(stderr, " %02x", cbuf->ptr[i]);

		if ((i+1) % 16 == 0)
			fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}
