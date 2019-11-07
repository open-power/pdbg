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

static int sbefifo_ring_get_push(uint32_t ring_addr, uint32_t ring_len_bits, uint16_t flags, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 5;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_RING | SBEFIFO_CMD_GET_RING;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(ring_addr);
	msg[3] = htobe32(ring_len_bits);
	msg[4] = htobe32(flags);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_ring_get_pull(uint8_t *buf, uint32_t buflen, uint32_t ring_len_bits, uint8_t **ring_data, uint32_t *ring_len)
{
	if (buflen*8 < ring_len_bits) {
		return EPROTO;
	}

	*ring_len = be32toh(*(uint32_t *) &buf[buflen-4]);
	*ring_data = malloc(*ring_len);
	if (! *ring_data)
		return ENOMEM;

	memcpy(*ring_data, buf, *ring_len);
	return 0;
}

int sbefifo_ring_get(struct sbefifo_context *sctx, uint32_t ring_addr, uint32_t ring_len_bits, uint16_t flags, uint8_t **ring_data, uint32_t *ring_len)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_ring_get_push(ring_addr, ring_len_bits, flags, &msg, &msg_len);
	if (rc)
		return rc;

	/* multiples of 64 bits */
	out_len = (ring_len_bits + 63) / 8;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_ring_get_pull(out, out_len, ring_len_bits, ring_data, ring_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_ring_put_push(uint16_t ring_mode, uint8_t *ring_data, uint32_t ring_data_len, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 3 + (ring_data_len + 3) / 4;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_RING | SBEFIFO_CMD_PUT_RING;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(ring_mode);
	memcpy(&msg[3], ring_data, ring_data_len);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_ring_put_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_ring_put(struct sbefifo_context *sctx, uint16_t ring_mode, uint8_t *ring_data, uint32_t ring_data_len)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_ring_put_push(ring_mode, ring_data, ring_data_len, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_ring_put_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_ring_put_from_image_push(uint16_t target, uint8_t chiplet_id, uint16_t ring_id, uint16_t ring_mode, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t target_word, ring_word;

	nwords = 4;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_RING | SBEFIFO_CMD_PUT_RING_IMAGE;

	target_word = ((uint32_t)target << 16) | (uint32_t)chiplet_id;
	ring_word = ((uint32_t)ring_id << 16) | (uint32_t)ring_mode;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target_word);
	msg[3] = htobe32(ring_word);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_ring_put_from_image_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_ring_put_from_image(struct sbefifo_context *sctx, uint16_t target, uint8_t chiplet_id, uint16_t ring_id, uint16_t ring_mode)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_ring_put_from_image_push(target, chiplet_id, ring_id, ring_mode, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(out);
	if (rc)
		return rc;

	rc = sbefifo_ring_put_from_image_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}
