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

int sbefifo_ring_get(struct sbefifo_context *sctx, uint32_t ring_addr, uint32_t ring_len_bits, uint16_t flags, uint8_t **ring_data, uint32_t *ring_len)
{
	uint8_t *out;
	uint32_t msg[5];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_RING | SBEFIFO_CMD_GET_RING;

	msg[0] = htobe32(5);	// number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(ring_addr);
	msg[3] = htobe32(ring_len_bits);
	msg[4] = htobe32(flags);

	/* multiples of 64 bits */
	out_len = (ring_len_bits + 63) / 8;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 5 * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len*8 < ring_len_bits) {
		free(out);
		return EPROTO;
	}

	*ring_len = be32toh(*(uint32_t *) &out[out_len-4]);
	*ring_data = malloc(*ring_len);
	if (! *ring_data) {
		free(out);
		return ENOMEM;
	}
	memcpy(*ring_data, out, *ring_len);

	free(out);
	return 0;
}

int sbefifo_ring_put(struct sbefifo_context *sctx, uint16_t ring_mode, uint8_t *ring_data, uint32_t ring_data_len)
{
	uint8_t *out;
	uint32_t nwords = (ring_data_len + 3) / 4;
	uint32_t msg[3+nwords];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_RING | SBEFIFO_CMD_PUT_RING;

	msg[0] = htobe32(3 + nwords); // number of words
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(ring_mode);
	memcpy(&msg[3], ring_data, ring_data_len);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, (3+nwords) * 4, cmd, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}

int sbefifo_ring_put_from_image(struct sbefifo_context *sctx, uint16_t target, uint8_t chiplet_id, uint16_t ring_id, uint16_t ring_mode)
{
	uint8_t *out;
	uint32_t msg[3];
	uint32_t cmd, out_len;
	uint32_t target_word, ring_word;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_RING | SBEFIFO_CMD_PUT_RING_IMAGE;
	target_word = ((uint32_t)target << 16) | (uint32_t)chiplet_id;
	ring_word = ((uint32_t)ring_id << 16) | (uint32_t)ring_mode;

	msg[0] = htobe32(3);	// number of words
	msg[1] = htobe32(target_word);
	msg[2] = htobe32(ring_word);

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
