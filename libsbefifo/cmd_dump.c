/* Copyright 2020 IBM Corp.
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
#include <string.h>

#include "libsbefifo.h"
#include "sbefifo_private.h"

static int sbefifo_get_dump_push(uint8_t type, uint8_t clock, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t flags;

	nwords =  3;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_DUMP | SBEFIFO_CMD_GET_DUMP;

	flags = ((uint32_t)(clock & 0x3) << 8) |
		 ((uint32_t)(type & 0xf));

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(flags);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_get_dump_pull(uint8_t *buf, uint32_t buflen, uint8_t **data, uint32_t *data_len)
{
	if (buflen > 0) {
		*data = malloc(buflen);
		if (! *data)
			return ENOMEM;

		memcpy(*data, buf, buflen);
		*data_len = buflen;
	} else {
		*data = NULL;
		*data_len = 0;
	}

	return 0;
}

int sbefifo_get_dump(struct sbefifo_context *sctx, uint8_t type, uint8_t clock, uint8_t **data, uint32_t *data_len)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_get_dump_push(type, clock, &msg, &msg_len);
	if (rc)
		return rc;

	/* dump size can be as large as 64MB */
	out_len = 64 * 1024 * 1024;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_get_dump_pull(out, out_len, data, data_len);
	if (out)
		free(out);

	return rc;
}
