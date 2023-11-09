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

static int sbefifo_register_get_push(uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint32_t *reg_id, uint8_t reg_count, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t r, i;

	if (reg_count == 0 || reg_count > 64)
		return EINVAL;

	nwords = 3 + reg_count;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_REGISTER | SBEFIFO_CMD_GET_REGISTER;

	r = ((uint32_t)(core_id & 0xff) << 16) |
	    ((uint32_t)(thread_id & 0x3) << 12) |
	    ((uint32_t)(reg_type & 0x3) << 8) |
	    ((uint32_t)(reg_count & 0xff));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(r);
	for (i=0; i<reg_count; i++)
		msg[3+i] = htobe32(reg_id[i]);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_register_get_pull(uint8_t *buf, uint32_t buflen, uint8_t reg_count, uint64_t **value)
{
	uint32_t i;
	uint32_t *b = (uint32_t *)buf;

	if (buflen != reg_count * 8)
		return EPROTO;

	*value = malloc(reg_count * 8);
	if (! *value)
		return ENOMEM;

	for (i=0; i<reg_count; i++) {
		uint32_t val1, val2;

		val1 = be32toh(b[i*2]);
		val2 = be32toh(b[i*2+1]);

		(*value)[i] = ((uint64_t)val1 << 32) | (uint64_t)val2;
	}

	return 0;
}

int sbefifo_register_get(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint32_t *reg_id, uint8_t reg_count, uint64_t **value)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_register_get_push(core_id, thread_id, reg_type, reg_id, reg_count, &msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc)
		return rc;

	out_len = reg_count * 8;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	sbefifo_reset_timeout(sctx);

	rc = sbefifo_register_get_pull(out, out_len, reg_count, value);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_register_put_push(uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint32_t *reg_id, uint8_t reg_count, uint64_t *value, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t r, i;

	if (reg_count == 0 || reg_count > 64)
		return EINVAL;

	nwords = 3 + 3*reg_count;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_REGISTER | SBEFIFO_CMD_PUT_REGISTER;

	r = ((uint32_t)(core_id & 0xff) << 16) |
	    ((uint32_t)(thread_id & 0x3) << 12) |
	    ((uint32_t)(reg_type & 0x3) << 8) |
	    ((uint32_t)(reg_count & 0xff));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(r);
	for (i=0; i<reg_count; i++) {
		msg[3+i*3] = htobe32(reg_id[i]);
		msg[3+i*3+1] = htobe32(value[i] >> 32);
		msg[3+i*3+2] = htobe32(value[i] & 0xffffffff);
	}

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_register_put_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_register_put(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint32_t *reg_id, uint8_t reg_count, uint64_t *value)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_register_put_push(core_id, thread_id, reg_type, reg_id, reg_count, value, &msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	sbefifo_reset_timeout(sctx);

	rc = sbefifo_register_put_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_hw_register_get_push(uint8_t target_type, uint8_t instance_id, uint64_t reg_id, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t target;

	nwords = 5;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_REGISTER | SBEFIFO_CMD_GET_HW_REGISTER;

	target = ((uint32_t)(target_type & 0xff) << 16) |
		 ((uint32_t)(instance_id & 0xff));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);
	msg[3] = htobe32(reg_id >> 32);
	msg[4] = htobe32(reg_id & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_hw_register_get_pull(uint8_t *buf, uint32_t buflen, uint64_t *value)
{
	uint32_t val1, val2;

	if (buflen != 8)
		return EPROTO;

	val1 = be32toh(*(uint32_t *) &buf[0]);
	val2 = be32toh(*(uint32_t *) &buf[4]);

	*value = ((uint64_t)val1 << 32) | (uint64_t)val2;

	return 0;
}

int sbefifo_hw_register_get(struct sbefifo_context *sctx, uint8_t target_type, uint8_t instance_id, uint64_t reg_id, uint64_t *value)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	if (sctx->proc == SBEFIFO_PROC_P9)
		return ENOSYS;

	rc = sbefifo_hw_register_get_push(target_type, instance_id, reg_id, &msg, &msg_len);
	printf("deepa sbefifo_hw_register_get_push rc: %d\n", rc);
	if (rc)
		return rc;
	out_len = 8;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	printf("deepa sbefifo_operation rc: %d", rc);
	if (rc)
		return rc;
	rc = sbefifo_hw_register_get_pull(out, out_len, value);
	printf("deepa sbefifo_hw_register_get_pull rc: %d", rc);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_hw_register_put_push(uint8_t target_type, uint8_t instance_id, uint64_t reg_id, uint64_t value, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t target;

	nwords = 7;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_REGISTER | SBEFIFO_CMD_PUT_HW_REGISTER;

	target = ((uint32_t)(target_type & 0xff) << 16) |
		 ((uint32_t)(instance_id & 0xff));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);
	msg[3] = htobe32(reg_id >> 32);
	msg[4] = htobe32(reg_id & 0xffffffff);
	msg[5] = htobe32(value >> 32);
	msg[6] = htobe32(value & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_hw_register_put_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_hw_register_put(struct sbefifo_context *sctx, uint8_t target_type, uint8_t instance_id, uint64_t reg_id, uint64_t value)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	if (sctx->proc == SBEFIFO_PROC_P9)
		return ENOSYS;

	rc = sbefifo_hw_register_put_push(target_type, instance_id, reg_id, value, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_hw_register_put_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}
