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

static int sbefifo_istep_execute_push(uint8_t major, uint8_t minor, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t step;

	nwords = 3;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_CONTROL | SBEFIFO_CMD_EXECUTE_ISTEP;

	step = ((uint32_t)major << 16) | (uint32_t)minor;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(step);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_istep_execute_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_istep_execute(struct sbefifo_context *sctx, uint8_t major, uint8_t minor)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_istep_execute_push(major, minor, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_istep_execute_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_suspend_io_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_CONTROL | SBEFIFO_CMD_SUSPEND_IO;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_suspend_io_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_suspend_io(struct sbefifo_context *sctx)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_suspend_io_push(&msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_suspend_io_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}
