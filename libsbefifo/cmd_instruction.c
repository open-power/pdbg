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

int sbefifo_control_insn(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t thread_op, uint8_t mode)
{
	uint8_t *out;
	uint32_t msg[3];
	uint32_t cmd, out_len;
	uint32_t oper;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_INSTRUCTION | SBEFIFO_CMD_CONTROL_INSN;
	oper = ((uint32_t)(mode & 0xf) << 16) |
		((uint32_t)(core_id & 0xff) << 8) |
		((uint32_t)(thread_id & 0xf) << 4) |
		((uint32_t)(thread_op & 0xf));

	msg[0] = htobe32(3);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(oper);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 3 * 4, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}
