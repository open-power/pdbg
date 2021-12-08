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

static int sbefifo_get_ffdc_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_GET_FFDC;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_get_ffdc_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen < 3*sizeof(uint32_t))
		return EPROTO;

	return 0;
}

int sbefifo_get_ffdc(struct sbefifo_context *sctx)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	uint32_t status;
	int rc;

	rc = sbefifo_get_ffdc_push(&msg, &msg_len);
	if (rc)
		return rc;

	/* We don't know how much data to expect, let's assume it's less than 32K */
	out_len = 0x8000;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_get_ffdc_pull(out, out_len);
	if (rc)
		return rc;

	status = SBEFIFO_PRI_UNKNOWN_ERROR | SBEFIFO_SEC_GENERIC_FAILURE;
	sbefifo_ffdc_set(sctx, status, out, out_len);
	return 0;
}

static int sbefifo_get_capabilities_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_GET_CAPABILITY;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_get_capabilities_pull(uint8_t *buf, uint32_t buflen, uint32_t *version, char **commit_id, char **release_tag, uint32_t **caps, uint32_t *caps_count)
{
	uint32_t i;

	if (buflen != 26 * sizeof(uint32_t))
		return EPROTO;

	*version = be32toh(*(uint32_t *) &buf[0]);

	*commit_id = malloc(9);
	if (! *commit_id)
		return ENOMEM;

	memcpy(*commit_id, &buf[4], 8);
	(*commit_id)[8] = '\0';

	*release_tag = malloc(21);
	if (! *release_tag)
		return ENOMEM;

	memcpy(*release_tag, &buf[12], 20);
	(*release_tag)[20] = '\0';

	*caps = malloc(20 * sizeof(uint32_t));
	if (! *caps)
		return ENOMEM;

	*caps_count = 20;
	for (i=0; i<20; i++)
		(*caps)[i] = be32toh(*(uint32_t *) &buf[32+i*4]);

	return 0;
}

int sbefifo_get_capabilities(struct sbefifo_context *sctx, uint32_t *version, char **commit_id, char **release_tag, uint32_t **caps, uint32_t *caps_count)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_get_capabilities_push(&msg, &msg_len);
	if (rc)
		return rc;

	/*
	 * Major/minor version - 1 word
	 * GIT Sha - 1 word
	 * Release tag - 5 words
	 * Capabilities - 20 words
	 */
	out_len = 27 * sizeof(uint32_t);
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_get_capabilities_pull(out, out_len, version, commit_id, release_tag, caps, caps_count);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_quiesce_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_QUIESCE;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_quiesce_pull(uint8_t *buf, uint32_t buflen)
{
	if (buflen != 0)
		return EPROTO;

	return 0;
}

int sbefifo_quiesce(struct sbefifo_context *sctx)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_quiesce_push(&msg, &msg_len);
	if (rc)
		return rc;

	out_len = 0;
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_quiesce_pull(out, out_len);
	if (out)
		free(out);

	return rc;
}

static int sbefifo_lpc_timeout_push(uint8_t **buf, uint32_t *buflen)
{
	uint32_t *msg;
	uint32_t nwords, cmd;

	nwords = 2;
	*buflen = nwords * sizeof(uint32_t);
	msg = malloc(*buflen);
	if (!msg)
		return ENOMEM;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_LPC_TIMEOUT;

	msg[0] = htobe32(nwords);
	msg[1] = htobe32(cmd);

	*buf = (uint8_t *)msg;
	return 0;
}

static int sbefifo_lpc_timeout_pull(uint8_t *buf, uint32_t buflen, uint32_t *timeout_flag)
{
	if (buflen != sizeof(uint32_t))
		return EPROTO;

	*timeout_flag = be32toh(*(uint32_t *) &buf[0]);

	return 0;
}

int sbefifo_lpc_timeout(struct sbefifo_context *sctx, uint32_t *timeout_flag)
{
	uint8_t *msg, *out;
	uint32_t msg_len, out_len;
	int rc;

	rc = sbefifo_lpc_timeout_push(&msg, &msg_len);
	if (rc)
		return rc;

	out_len = 1 * sizeof(uint32_t);
	rc = sbefifo_operation(sctx, msg, msg_len, &out, &out_len);
	free(msg);
	if (rc)
		return rc;

	rc = sbefifo_lpc_timeout_pull(out, out_len, timeout_flag);
	if (out)
		free(out);

	return rc;
}
