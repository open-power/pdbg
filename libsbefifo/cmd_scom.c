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

static int sbefifo_scom_get_push(uint64_t addr, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 4;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_GET_SCOM;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(addr >> 32);
	msg[3] = htobe32(addr & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_scom_get_pull(uint8_t *buf, uint32_t buflen, uint64_t *value)
{
	uint32_t val1, val2;

	if (buflen != 2 * sizeof(uint32_t))
		return EPROTO;

	val1 = be32toh(*(uint32_t *) &buf[0]);
	val2 = be32toh(*(uint32_t *) &buf[4]);

	*value = ((uint64_t)val1 << 32) | (uint64_t)val2;
	return 0;
}

int sbefifo_scom_get(struct sbefifo_context *sctx, uint64_t addr, uint64_t *value)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_scom_get_push(addr, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 2 * sizeof(uint32_t);
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_scom_get_pull(out, out_len, value);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_scom_put_push(uint64_t addr, uint64_t value, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 6;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_PUT_SCOM;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(addr >> 32);
	msg[3] = htobe32(addr & 0xffffffff);
	msg[2] = htobe32(value >> 32);
	msg[3] = htobe32(value & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_scom_put_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_scom_put(struct sbefifo_context *sctx, uint64_t addr, uint64_t value)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_scom_put_push(addr, value, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_scom_put_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_scom_modify_push(uint64_t addr, uint64_t value, uint8_t operand, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 7;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_MODIFY_SCOM;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(operand);
	msg[3] = htobe32(addr >> 32);
	msg[4] = htobe32(addr & 0xffffffff);
	msg[5] = htobe32(value >> 32);
	msg[6] = htobe32(value & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_scom_modify_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_scom_modify(struct sbefifo_context *sctx, uint64_t addr, uint64_t value, uint8_t operand)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_scom_modify_push(addr, value, operand, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_scom_modify_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_scom_put_mask_push(uint64_t addr, uint64_t value, uint64_t mask, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 8;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_SCOM | SBEFIFO_CMD_PUT_SCOM_MASK;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(addr >> 32);
	msg[3] = htobe32(addr & 0xffffffff);
	msg[4] = htobe32(value >> 32);
	msg[5] = htobe32(value & 0xffffffff);
	msg[6] = htobe32(mask >> 32);
	msg[7] = htobe32(mask & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_scom_put_mask_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_scom_put_mask(struct sbefifo_context *sctx, uint64_t addr, uint64_t value, uint64_t mask)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_scom_put_mask_push(addr, value, mask, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_scom_put_mask_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}
