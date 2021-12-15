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

static int sbefifo_mpipl_enter_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_MPIPL | SBEFIFO_CMD_ENTER_MPIPL;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_mpipl_enter_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_mpipl_enter(struct sbefifo_context *sctx)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_mpipl_enter_push(&msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc) {
		free(msg);
		return rc;
	}

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	sbefifo_reset_timeout(sctx);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_mpipl_enter_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_mpipl_continue_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_MPIPL | SBEFIFO_CMD_CONTINUE_MPIPL;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_mpipl_continue_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_mpipl_continue(struct sbefifo_context *sctx)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_mpipl_continue_push(&msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc) {
		free(msg);
		return rc;
	}

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	sbefifo_reset_timeout(sctx);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_mpipl_continue_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_mpipl_stopclocks_push(uint16_t target_type, uint8_t chiplet_id, uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;
	uint32_t target;

	nwords = 3;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_MPIPL | SBEFIFO_CMD_STOP_CLOCKS;

	target = ((uint32_t)target_type << 16) | chiplet_id;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);
	msg[2] = htobe32(target);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_mpipl_stopclocks_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_mpipl_stopclocks(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_mpipl_stopclocks_push(target_type, chiplet_id, &msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_mpipl_stopclocks_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_mpipl_get_ti_info_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_MPIPL | SBEFIFO_CMD_GET_TI_INFO;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_mpipl_get_ti_info_pull(uint8_t *buf, uint32_t buflen, uint8_t **data, uint32_t *data_len)
{
	if (buflen < 4)
		return EPROTO;

	*data_len = be32toh(*(uint32_t *) &buf[buflen-4]);
	*data = malloc(*data_len);
	if (! *data)
		return ENOMEM;

	memcpy(*data, buf, *data_len);
	return 0;
}

int sbefifo_mpipl_get_ti_info(struct sbefifo_context *sctx, uint8_t **data, uint32_t *data_len)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_mpipl_get_ti_info_push(&msg, &msg_len);
	if (rc)
		return rc;

	rc = sbefifo_set_long_timeout(sctx);
	if (rc) {
		free(msg);
		return rc;
	}

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	sbefifo_reset_timeout(sctx);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_mpipl_get_ti_info_pull(out, out_len, data, data_len);
	if (out)
		free(out);

	return rc;
}
