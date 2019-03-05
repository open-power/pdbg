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

#ifndef __LIBCRONUS_BUFFER_H__
#define __LIBCRONUS_BUFFER_H__

#include <stdint.h>

struct cronus_buffer {
	uint8_t *ptr;
	size_t size;
	size_t offset;
};

int cbuf_new(struct cronus_buffer *cbuf, size_t size);
int cbuf_new_from_buf(struct cronus_buffer *cbuf, uint8_t *ptr, size_t size);
void cbuf_free(struct cronus_buffer *cbuf);
void cbuf_init(struct cronus_buffer *cbuf, uint8_t *ptr, size_t size);

uint8_t *cbuf_finish(struct cronus_buffer *cbuf, size_t *length);

uint8_t *cbuf_ptr(struct cronus_buffer *cbuf);
size_t cbuf_offset(struct cronus_buffer *cbuf);
size_t cbuf_size(struct cronus_buffer *cbuf);

void cbuf_write(struct cronus_buffer *cbuf, uint8_t *bytes, size_t count);
void cbuf_read(struct cronus_buffer *cbuf, uint8_t *bytes, size_t count);

void cbuf_write_uint8(struct cronus_buffer *cbuf, uint8_t value);
void cbuf_read_uint8(struct cronus_buffer *cbuf, uint8_t *value);

void cbuf_write_uint16(struct cronus_buffer *cbuf, uint16_t value);
void cbuf_read_uint16(struct cronus_buffer *cbuf, uint16_t *value);

void cbuf_write_uint32(struct cronus_buffer *cbuf, uint32_t value);
void cbuf_read_uint32(struct cronus_buffer *cbuf, uint32_t *value);

void cbuf_write_uint64(struct cronus_buffer *cbuf, uint64_t value);
void cbuf_read_uint64(struct cronus_buffer *cbuf, uint64_t *value);

void cbuf_dump(struct cronus_buffer *cbuf, const char *prefix);

#endif /* __LIBCRONUS_BUFFER_H__ */
