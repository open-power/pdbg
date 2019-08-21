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

int sbefifo_mpipl_enter(struct sbefifo_context *sctx)
{
	uint8_t *out;
	uint32_t msg[2];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_MPIPL | SBEFIFO_CMD_ENTER_MPIPL;

	msg[0] = htobe32(2);	// number of words
	msg[1] = htobe32(cmd);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 2 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}

int sbefifo_mpipl_continue(struct sbefifo_context *sctx)
{
	uint8_t *out;
	uint32_t msg[2];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_MPIPL | SBEFIFO_CMD_CONTINUE_MPIPL;

	msg[0] = htobe32(2);	// number of words
	msg[1] = htobe32(cmd);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 2 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}

int sbefifo_mpipl_stopclocks(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id)
{
	uint8_t *out;
	uint32_t msg[3];
	uint32_t cmd, out_len;
	uint32_t target;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_STOP_CLOCKS;
	target = ((uint32_t)target_type << 16) | chiplet_id;

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
