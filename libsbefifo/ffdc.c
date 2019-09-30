/* Copyright 2016 IBM Corp.
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
#include <endian.h>

#include "sbefifo_private.h"

void sbefifo_ffdc_clear(struct sbefifo_context *sctx)
{
	sctx->status = 0;

	if (sctx->ffdc) {
		free(sctx->ffdc);
		sctx->ffdc = NULL;
		sctx->ffdc_len = 0;
	}
}

void sbefifo_ffdc_set(struct sbefifo_context *sctx, uint32_t status, uint8_t *ffdc, uint32_t ffdc_len)
{
	sctx->status = status;

	sctx->ffdc = malloc(ffdc_len);
	if (!sctx->ffdc) {
		fprintf(stderr, "Memory allocation error\n");
		return;
	}

	memcpy(sctx->ffdc, ffdc, ffdc_len);
	sctx->ffdc_len = ffdc_len;
}

uint32_t sbefifo_ffdc_get(struct sbefifo_context *sctx, const uint8_t **ffdc, uint32_t *ffdc_len)
{
	*ffdc = sctx->ffdc;
	*ffdc_len = sctx->ffdc_len;

	return sctx->status;
}

static int sbefifo_ffdc_get_uint32(struct sbefifo_context *sctx, uint32_t offset, uint32_t *value)
{
	uint32_t v;

	if (offset + 4 > sctx->ffdc_len)
		return -1;

	memcpy(&v, sctx->ffdc + offset, 4);
	*value = be32toh(v);

	return 0;
}

static int sbefifo_ffdc_dump_pkg(struct sbefifo_context *sctx, uint32_t offset)
{
	uint32_t offset2 = offset;
	uint32_t header, value;
	uint16_t magic, len_words;
	int i, rc;

	rc = sbefifo_ffdc_get_uint32(sctx, offset2, &header);
	if (rc < 0)
		return -1;
	offset2 += 4;

	/*
	 * FFDC package structure
	 *
	 *             +----------+----------+----------+----------+
	 *             |  Byte 0  |  Byte 1  |  Byte 2  |  Byte  3 |
	 *  +----------+----------+----------+----------+----------+
	 *  |  word 0  |        magic        | length in words (N) |
	 *  +----------+---------------------+---------------------+
	 *  |  word 1  |      sequence id    |       command       |
	 *  +----------+---------------------+---------------------+
	 *  |  word 2  |              return code                  |
	 *  +----------+-------------------------------------------+
	 *  |  word 3  |            FFDC Data - word 0             |
	 *  +----------+-------------------------------------------+
	 *  |  word 4  |            FFDC Data - word 1             |
	 *  +----------+-------------------------------------------+
	 *  |    ...   |                    ...                    |
	 *  +----------+-------------------------------------------+
	 *  |  word N  |            FFDC Data - word N-3           |
	 *  +----------+----------+----------+----------+----------+
	 */

	magic = header >> 16;
	if (magic != 0xffdc) {
		fprintf(stderr, "sbefifo: ffdc expected 0xffdc, got 0x%04x\n", magic);
		return -1;
	}

	len_words = header & 0xffff;

	rc = sbefifo_ffdc_get_uint32(sctx, offset2, &value);
	if (rc < 0)
		return -1;
	offset2 += 4;

	printf("FFDC: Sequence = %u\n", value >> 16);
	printf("FFDC: Command = 0x%08x\n", value & 0xffff);

	rc = sbefifo_ffdc_get_uint32(sctx, offset2, &value);
	if (rc < 0)
		return -1;
	offset2 += 4;

	printf("FFDC: RC = 0x%08x\n", value);

	for (i=0; i<len_words-3; i++) {
		rc = sbefifo_ffdc_get_uint32(sctx, offset2, &value);
		if (rc < 0)
			return -1;
		offset2 += 4;

		printf("FFDC: Data: 0x%08x\n", value);
	}

	return offset2 - offset;
}

void sbefifo_ffdc_dump(struct sbefifo_context *sctx)
{
	uint32_t offset = 0;

	while (offset < sctx->ffdc_len) {
		int n;

		n = sbefifo_ffdc_dump_pkg(sctx, offset);
		if (n <= 0)
			break;

		offset += n;
	}
}
