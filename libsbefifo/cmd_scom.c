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
#include <endian.h>
#include <errno.h>

#include "libsbefifo.h"
#include "sbefifo_private.h"

int sbefifo_scom_get(struct sbefifo_context *sctx, uint64_t addr, uint64_t *value)
{
	uint8_t *out;
	uint32_t msg[4];
	uint32_t cmd, out_len;
	uint32_t val1, val2;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_GET_SCOM;

	msg[0] = htobe32(4);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(addr >> 32);
	msg[3] = htobe32(addr & 0xffffffff);

	out_len = 2 * 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 4 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 2 * 4) {
		free(out);
		return EPROTO;
	}

	val1 = be32toh(*(uint32_t *) &out[0]);
	val2 = be32toh(*(uint32_t *) &out[4]);
	free(out);

	*value = ((uint64_t)val1 << 32) | (uint64_t)val2;

	return 0;
}

int sbefifo_scom_put(struct sbefifo_context *sctx, uint64_t addr, uint64_t value)
{
	uint8_t *out;
	uint32_t msg[6];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_PUT_SCOM;

	msg[0] = htobe32(6);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(addr >> 32);
	msg[3] = htobe32(addr & 0xffffffff);
	msg[2] = htobe32(value >> 32);
	msg[3] = htobe32(value & 0xffffffff);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 6 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}

int sbefifo_scom_modify(struct sbefifo_context *sctx, uint64_t addr, uint64_t value, uint8_t operand)
{
	uint8_t *out;
	uint32_t msg[7];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_MODIFY_SCOM;

	msg[0] = htobe32(7);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(operand);
	msg[3] = htobe32(addr >> 32);
	msg[4] = htobe32(addr & 0xffffffff);
	msg[5] = htobe32(value >> 32);
	msg[6] = htobe32(value & 0xffffffff);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 7 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}

int sbefifo_scom_put_mask(struct sbefifo_context *sctx, uint64_t addr, uint64_t value, uint64_t mask)
{
	uint8_t *out;
	uint32_t msg[8];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_PUT_SCOM_MASK;

	msg[0] = htobe32(8);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(addr >> 32);
	msg[3] = htobe32(addr & 0xffffffff);
	msg[4] = htobe32(value >> 32);
	msg[5] = htobe32(value & 0xffffffff);
	msg[6] = htobe32(mask >> 32);
	msg[7] = htobe32(mask & 0xffffffff);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 8 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}
