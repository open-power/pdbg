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

static int sbefifo_control_insn_push(uint8_t core_id, uint8_t thread_id, uint8_t thread_op, uint8_t mode, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t oper;

	nwords = 3;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_INSTRUCTION | SBEFIFO_CMD_CONTROL_INSN;

	oper = ((uint32_t)(mode & 0xf) << 16) |
		((uint32_t)(core_id & 0xff) << 8) |
		((uint32_t)(thread_id & 0xf) << 4) |
		((uint32_t)(thread_op & 0xf));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(oper);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_control_insn_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_control_insn(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t thread_op, uint8_t mode)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_control_insn_push(core_id, thread_id, thread_op, mode, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_control_insn_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}
