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

int sbefifo_istep_execute(struct sbefifo_context *sctx, uint8_t major, uint8_t minor)
{
	uint8_t *out;
	uint32_t msg[3];
	uint32_t cmd, step, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_CONTROL | SBEFIFO_CMD_EXECUTE_ISTEP;
	step = (major << 16) | minor;

	msg[0] = htobe32(3);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(step);

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
