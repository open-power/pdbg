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
#include <errno.h>

#include "libsbefifo.h"
#include "sbefifo_private.h"

static int sbefifo_control_fast_array_push(uint16_t target_type, uint8_t chiplet_id, uint8_t mode, uint64_t clock_cycle, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t target;

	nwords = 5;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_ARRAY | SBEFIFO_CMD_FAST_ARRAY;

	target = ((uint32_t)target_type << 16) |
		 ((uint32_t)chiplet_id << 8) |
		 ((uint32_t)(mode & 0x3));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);
	msg[3] = htobe32(clock_cycle >> 32);
	msg[4] = htobe32(clock_cycle & 0xffffffff);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_control_fast_array_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_control_fast_array(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id, uint8_t mode, uint64_t clock_cycle)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_control_fast_array_push(target_type, chiplet_id, mode, clock_cycle, &msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc) {
		free(msg);
		return rc;
	}

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	sbefifo_reset_timeout(sctx);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_control_fast_array_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_control_trace_array_push(uint16_t target_type, uint8_t chiplet_id, uint16_t array_id, uint16_t operation, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t target, oper;

	nwords = 4;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_ARRAY | SBEFIFO_CMD_TRACE_ARRAY;

	target = ((uint32_t)target_type << 16) | ((uint32_t)chiplet_id);
	oper = ((uint32_t)array_id << 16) | ((uint32_t)operation);

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);
	msg[3] = htobe32(oper);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_control_trace_array_pull(uint8_t *buf, uint32_t buflen, uint8_t **trace_data, uint32_t *trace_data_len)
{
	if (buflen < 4)
		return EPROTO;

	*trace_data_len = 4 * be32toh(*(uint32_t *) &buf[buflen-4]);

	if (buflen != *trace_data_len + 4)
		return EPROTO;

	*trace_data = malloc(*trace_data_len);
	if (! *trace_data)
		return ENOMEM;

	memcpy(*trace_data, buf, *trace_data_len);
	return 0;
}

int sbefifo_control_trace_array(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id, uint16_t array_id, uint16_t operation, uint8_t **trace_data, uint32_t *trace_data_len)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_control_trace_array_push(target_type, chiplet_id, array_id, operation, &msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc) {
		free(msg);
		return rc;
	}

	/* The size of returned data is just a guess */
	out_len = 16 * sizeof(uint32_t);
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	sbefifo_reset_timeout(sctx);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_control_trace_array_pull(out, out_len, trace_data, trace_data_len);
	if (out)
		free(out);

	return rc;
}
