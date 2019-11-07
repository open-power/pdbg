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

int sbefifo_register_get(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint8_t *reg_id, uint8_t reg_count, uint64_t **value)
{
	uint8_t *out;
	uint32_t msg[3+reg_count];
	uint32_t cmd, out_len;
	uint32_t r, i;
	int rc;

	if (reg_count == 0 || reg_count > 64)
		return EINVAL;

	cmd = SBEFIFO_CMD_CLASS_REGISTER | SBEFIFO_CMD_GET_REGISTER;
	r = ((uint32_t)(core_id & 0xff) << 16) |
	    ((uint32_t)(thread_id & 0x3) << 12) |
	    ((uint32_t)(reg_type & 0x3) << 8) |
	    ((uint32_t)(reg_count & 0xff));

	msg[0] = htobe32(3+reg_count);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(r);
	for (i=0; i<reg_count; i++) {
		msg[3+i] = htobe32(reg_id[i] & 0xff);
	}

	out_len = reg_count * 8;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, (3+reg_count) * 4, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != reg_count * 8) {
		free(out);
		return EPROTO;
	}

	*value = malloc(reg_count * 8);
	if (! *value) {
		free(out);
		return ENOMEM;
	}

	for (i=0; i<reg_count; i++) {
		uint32_t val1, val2;

		val1 = be32toh(*(uint32_t *) &out[i*4]);
		val2 = be32toh(*(uint32_t *) &out[i*4+4]);

		(*value)[i] = ((uint64_t)val1 << 32) | (uint64_t)val2;
	}

	free(out);
	return 0;
}

int sbefifo_register_put(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint8_t *reg_id, uint8_t reg_count, uint64_t *value)
{
	uint8_t *out;
	uint32_t msg[3+(3*reg_count)];
	uint32_t cmd, out_len;
	uint32_t r, i;
	int rc;

	if (reg_count == 0 || reg_count > 64)
		return EINVAL;

	cmd = SBEFIFO_CMD_CLASS_REGISTER | SBEFIFO_CMD_PUT_REGISTER;
	r = ((uint32_t)(core_id & 0xff) << 16) |
	    ((uint32_t)(thread_id & 0x3) << 12) |
	    ((uint32_t)(reg_type & 0x3) << 8) |
	    ((uint32_t)(reg_count & 0xff));

	msg[0] = htobe32(3 + 3*reg_count);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(r);
	for (i=0; i<reg_count; i++) {
		msg[3+i*3] = htobe32(reg_id[i] & 0xff);
		msg[3+i*3+1] = htobe32(value[i] >> 32);
		msg[3+i*3+2] = htobe32(value[i] & 0xffffffff);
	}

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, (3+3*reg_count) * 4, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}
