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

int sbefifo_control_fast_array(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id, uint8_t mode, uint64_t clock_cycle)
{
	uint8_t *out;
	uint32_t msg[3];
	uint32_t cmd, out_len;
	uint32_t target;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_ARRAY | SBEFIFO_CMD_FAST_ARRAY;
	target = ((uint32_t)target_type << 16) |
		 ((uint32_t)chiplet_id << 8) |
		 ((uint32_t)(mode & 0x3));

	msg[0] = htobe32(3);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 3 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}

int sbefifo_control_trace_array(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id, uint16_t array_id, uint16_t operation, uint8_t **trace_data, uint32_t *trace_data_len)
{
	uint8_t *out;
	uint32_t msg[4];
	uint32_t cmd, out_len;
	uint32_t target, oper;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_ARRAY | SBEFIFO_CMD_TRACE_ARRAY;
	target = ((uint32_t)target_type << 16) | ((uint32_t)chiplet_id);
	oper = ((uint32_t)array_id << 16) | ((uint32_t)operation);

	msg[0] = htobe32(4);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);
	msg[3] = htobe32(oper);

	out_len = 16 * 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 4 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len < 4) {
		free(out);
		return EPROTO;
	}

	*trace_data_len = 4 * be32toh(*(uint32_t *) &out[out_len-4]);

	if (out_len != *trace_data_len + 4) {
		free(out);
		return EPROTO;
	}

	*trace_data = malloc(*trace_data_len);
	if (! *trace_data) {
		free(out);
		return ENOMEM;
	}

	memcpy(*trace_data, out, *trace_data_len);

	free(out);
	return 0;
}
