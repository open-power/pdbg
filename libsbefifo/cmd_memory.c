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

static int sbefifo_mem_get_push(uint64_t addr, uint32_t size, uint16_t flags, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint64_t start_addr, end_addr;
	uint32_t nwords, cmd;
	uint32_t align, len;

	if (flags & SBEFIFO_MEMORY_FLAG_PROC)
		align = 8;
	else if (flags & SBEFIFO_MEMORY_FLAG_PBA)
		align = 128;
	else
		return EINVAL;

	start_addr = addr & (~(uint64_t)(align-1));
	end_addr = (addr + size + (align-1)) & (~(uint64_t)(align-1));

	if (end_addr - start_addr > UINT32_MAX)
		return EINVAL;

	nwords = 6;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	len = end_addr - start_addr;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_GET_MEMORY;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);
	msg[3] = htobe32(start_addr >> 32);
	msg[4] = htobe32(start_addr & 0xffffffff);
	msg[5] = htobe32(len);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_mem_get_pull(uint8_t *buf, uint32_t buflen, uint64_t addr, uint32_t size, uint32_t flags, uint8_t **data)
{
	uint64_t start_addr, offset;
	uint32_t align, len, i, j;
	bool do_tag = false, do_ecc = false;

	if (buflen < 4)
		return EPROTO;

	len = be32toh(*(uint32_t *) &buf[buflen-4]);
	*data = malloc(len);
	if (! *data)
		return ENOMEM;

	if (flags & SBEFIFO_MEMORY_FLAG_ECC_REQ)
		do_ecc = true;

	if (flags & SBEFIFO_MEMORY_FLAG_TAG_REQ)
		do_tag = true;

	if (flags & SBEFIFO_MEMORY_FLAG_PROC)
		align = 8;
	else if (flags & SBEFIFO_MEMORY_FLAG_PBA)
		align = 128;
	else
		return EINVAL;

	i = 0;
	j = 0;
	while (i < len) {
		memcpy((void *)&(*data)[j], (void *)&buf[i], 8);
		i += 8;
		j += 8;

		if (do_tag)
			i++;

		if (do_ecc)
			i++;
	}

	start_addr = addr & (~(uint64_t)(align-1));
	offset = addr - start_addr;
	if (offset)
		memmove(*data, *data + offset, size);

	return 0;
}

int sbefifo_mem_get(struct sbefifo_context *sctx, uint64_t addr, uint32_t size, uint16_t flags, uint8_t **data)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	uint32_t len, extra_bytes;
	int rc;

	rc = sbefifo_mem_get_push(addr, size, flags, &msg, &msg_len);
	if (rc)
		return rc;

	/* length is 6th word in the request */
	len = be32toh(*(uint32_t *)(msg + 20));
	extra_bytes = 0;

	if (flags & SBEFIFO_MEMORY_FLAG_ECC_REQ)
		extra_bytes = len / 8;

	if (flags & SBEFIFO_MEMORY_FLAG_TAG_REQ)
		extra_bytes = len / 8;

	out_len = len + extra_bytes + 4;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;


	rc = sbefifo_mem_get_pull(out, out_len, addr, size, flags, data);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_mem_put_push(uint64_t addr, uint8_t *data, uint32_t data_len, uint16_t flags, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t align;

	if (flags & SBEFIFO_MEMORY_FLAG_PROC)
		align = 8;
	else if (flags & SBEFIFO_MEMORY_FLAG_PBA)
		align = 128;
	else
		return EINVAL;

	if (addr & (align-1))
		return EINVAL;

	if (data_len & (align-1))
		return EINVAL;

	nwords = 6 + data_len/4;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_PUT_MEMORY;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);
	msg[3] = htobe32(addr >> 32);
	msg[4] = htobe32(addr & 0xffffffff);
	msg[5] = htobe32(data_len);
	memcpy(&msg[6], data, data_len);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_mem_put_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != sizeof(uint32_t))
		return EPROTO;

	return 0;
}

int sbefifo_mem_put(struct sbefifo_context *sctx, uint64_t addr, uint8_t *data, uint32_t data_len, uint16_t flags)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_mem_put_push(addr, data, data_len, flags, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 1 * 4;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_mem_put_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_sram_get_push(uint16_t chiplet_id, uint64_t addr, uint32_t size, uint8_t mode, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd, flags;
	uint64_t start_addr, end_addr;
	uint32_t align, len;

	align = 8;
	start_addr = addr & (~(uint32_t)(align-1));
	end_addr = (addr + size + (align-1)) & (~(uint32_t)(align-1));

	if (end_addr < start_addr)
		return EINVAL;

	nwords = 6;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	len = (end_addr - start_addr) / 8;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_GET_SRAM;

	flags = ((uint32_t)chiplet_id << 16 | (uint32_t)mode);

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);
	msg[3] = htobe32(start_addr >> 32);
	msg[4] = htobe32(start_addr & 0xffffffff);
	msg[5] = htobe32(len);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_sram_get_pull(uint8_t *buf, uint32_t buflen, uint64_t addr, uint32_t size, uint8_t **data, uint32_t *data_len)
{
	uint64_t start_addr;
	uint32_t align, offset;

	if (buflen < 4)
		return EPROTO;

	*data_len = be32toh(*(uint32_t *) &buf[buflen-4]);
	if (*data_len < size)
		return EPROTO;

	*data = malloc(size);
	if (! *data)
		return ENOMEM;

	align = 8;
	start_addr = addr & (~(uint32_t)(align-1));
	offset = addr - start_addr;

	memcpy(*data, buf + offset, size);

	return 0;
}

int sbefifo_sram_get(struct sbefifo_context *sctx, uint16_t chiplet_id, uint64_t addr, uint32_t size, uint8_t mode, uint8_t **data, uint32_t *data_len)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	uint32_t len;
	int rc;

	if (sctx->proc == SBEFIFO_PROC_P9)
		return ENOSYS;

	rc = sbefifo_sram_get_push(chiplet_id, addr, size, mode, &msg, &msg_len);
	if (rc)
		return rc;

	/* length is 6th word in the request */
	len = be32toh(*(uint32_t *)(msg + 20));

	out_len = len + 4;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_sram_get_pull(out, out_len, addr, size, data, data_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_sram_put_push(uint16_t chiplet_id, uint64_t addr, uint8_t *data, uint32_t data_len, bool multicast, uint8_t mode, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd, flags;
	uint32_t align, len;
	uint8_t multicast_bit;

	align = 8;

	if (addr & (align-1))
		return EINVAL;

	if (data_len & (align-1))
		return EINVAL;

	nwords = 6 + data_len/4;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	multicast_bit = multicast ? 0x80 : 0x00;

	cmd = SBEFIFO_CMD_CLASS_MEMORY | SBEFIFO_CMD_PUT_SRAM;

	flags = ((uint32_t)chiplet_id << 16) |
		((uint32_t)multicast_bit << 8) |
		(uint32_t)mode;

	len = data_len / 8;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);
	msg[3] = htobe32(addr >> 32);
	msg[4] = htobe32(addr & 0xffffffff);
	msg[5] = htobe32(len);
	memcpy(&msg[5], data, data_len);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_sram_put_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != sizeof(uint32_t))
		return EPROTO;

	return 0;
}

int sbefifo_sram_put(struct sbefifo_context *sctx, uint16_t chiplet_id, uint64_t addr, uint8_t *data, uint32_t data_len, bool multicast, uint8_t mode)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	if (sctx->proc == SBEFIFO_PROC_P9)
		return ENOSYS;

	rc = sbefifo_sram_put_push(chiplet_id, addr, data, data_len, multicast, mode, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 4;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_sram_put_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}
