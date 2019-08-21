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
#include <endian.h>
#include <stdbool.h>
#include <errno.h>

#include "libsbefifo.h"
#include "sbefifo_private.h"

int sbefifo_mem_get(struct sbefifo_context *sctx, uint64_t addr, uint32_t size, uint16_t flags, uint8_t **data)
{
	uint8_t *out;
	uint64_t start_addr, end_addr;
	uint32_t msg[6];
	uint32_t cmd, out_len;
	uint32_t align, offset, len, extra_bytes, i, j;
	int rc;
	bool do_tag = false, do_ecc = false;

	if (flags & SBEFIFO_MEMORY_FLAG_PROC) {
		align = 8;

		if (flags & SBEFIFO_MEMORY_FLAG_ECC_REQ)
			do_ecc = true;

		if (flags & SBEFIFO_MEMORY_FLAG_TAG_REQ)
			do_tag = true;

	} else if (flags & SBEFIFO_MEMORY_FLAG_PBA) {
		align = 128;
	} else {
		return EINVAL;
	}

	start_addr = addr & (~(uint64_t)(align-1));
	end_addr = (addr + size + (align-1)) & (~(uint64_t)(align-1));

	if (end_addr - start_addr > 0xffffffff)
		return EINVAL;

	offset = addr - start_addr;
	len = end_addr - start_addr;

	extra_bytes = 0;
	if (do_tag)
		extra_bytes = len / 8;

	if (do_ecc)
		extra_bytes = len / 8;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_GET_MEMORY;

	msg[0] = htobe32(6);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);
	msg[3] = htobe32(start_addr >> 32);
	msg[4] = htobe32(start_addr & 0xffffffff);
	msg[5] = htobe32(len);

	out_len = len + extra_bytes + 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 6 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != len + extra_bytes + 4) {
		free(out);
		return EPROTO;
	}

	len = be32toh(*(uint32_t *) &out[out_len-4]);
	*data = malloc(len);
	if (! *data) {
		free(out);
		return ENOMEM;
	}

	i = 0;
	j = 0;
	while (i < len) {
		memcpy((void *)&(*data)[j], (void *)&out[i], 8);
		i += 8;
		j += 8;

		if (do_tag)
			i++;

		if (do_ecc)
			i++;
	}
	if (i < len)
		memcpy((void *)&(*data)[j], (void *)&out[i], len - i);

	memmove(*data, *data + offset, size);

	free(out);
	return 0;
}

int sbefifo_mem_put(struct sbefifo_context *sctx, uint64_t addr, uint8_t *data, uint32_t data_len, uint16_t flags)
{
	uint8_t *out;
	uint32_t nwords = (data_len+3)/4;
	uint32_t msg[6+nwords];
	uint32_t cmd, out_len;
	uint32_t align;
	int rc;

	if (flags & SBEFIFO_MEMORY_FLAG_PROC)
		align = 8;
	else if (flags & SBEFIFO_MEMORY_FLAG_PBA)
		align = 128;
	else
		return EINVAL;

	if (addr & (align-1))
		return EINVAL;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_PUT_MEMORY;

	msg[0] = htobe32(6 + nwords); // number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);
	msg[3] = htobe32(addr >> 32);
	msg[4] = htobe32(addr & 0xffffffff);
	msg[5] = htobe32(data_len);
	memcpy(&msg[6], data, data_len);

	out_len = 1 * 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, (6+nwords) * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 4) {
		free(out);
		return EPROTO;
	}

	free(out);
	return 0;
}

int sbefifo_occsram_get(struct sbefifo_context *sctx, uint32_t addr, uint32_t size, uint8_t mode, uint8_t **data, uint32_t *data_len)
{
	uint8_t *out;
	uint32_t start_addr, end_addr;
	uint32_t msg[5];
	uint32_t cmd, out_len;
	uint32_t align, offset, len;
	int rc;

	align = 8;
	start_addr = addr & (~(uint32_t)(align-1));
	end_addr = (addr + size + (align-1)) & (~(uint32_t)(align-1));

	offset = addr - start_addr;
	len = end_addr - start_addr;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_GET_OCCSRAM;

	msg[0] = htobe32(5);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(mode);
	msg[3] = htobe32(start_addr);
	msg[4] = htobe32(len);

	out_len = len + 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 5 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != len + 4) {
		free(out);
		return EPROTO;
	}

	*data_len = be32toh(*(uint32_t *) &out[out_len-4]);
	*data = malloc(*data_len);
	if (! *data) {
		free(out);
		return ENOMEM;
	}
	memcpy(*data, out + offset, size);

	free(out);
	return 0;
}

int sbefifo_occsram_put(struct sbefifo_context *sctx, uint32_t addr, uint8_t *data, uint32_t data_len, uint8_t mode)
{
	uint8_t *out;
	uint32_t nwords = (data_len+3)/4;
	uint32_t msg[5+nwords];
	uint32_t cmd, out_len;
	uint32_t align;
	int rc;

	align = 8;

	if (addr & (align-1))
		return EINVAL;

	if (data_len & (align-1))
		return EINVAL;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_PUT_OCCSRAM;

	msg[0] = htobe32(5 + nwords); // number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(mode);
	msg[3] = htobe32(addr);
	msg[4] = htobe32(data_len);
	memcpy(&msg[6], data, data_len);

	out_len = 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, (5+nwords) * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 4) {
		free(out);
		return EPROTO;
	}

	free(out);
	return 0;
}
