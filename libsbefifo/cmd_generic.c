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

int sbefifo_get_ffdc(struct sbefifo_context *sctx, uint8_t **ffdc, uint32_t *ffdc_len)
{
	uint8_t *out;
	uint32_t msg[2];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_GET_FFDC;

	msg[0] = htobe32(2);	// number of words
	msg[1] = htobe32(cmd);

	/* We don't know how much data to expect, let's assume it's less than 32K */
	out_len = 0x8000;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 2 * 4, &out, &out_len);
	if (rc)
		return rc;

	*ffdc_len = out_len;
	if (out_len > 0) {
		*ffdc = malloc(out_len);
		if (! *ffdc) {
			free(out);
			return ENOMEM;
		}
		memcpy(*ffdc, out, out_len);

		free(out);
	} else {
		*ffdc = NULL;
	}

	return 0;
}

int sbefifo_get_capabilities(struct sbefifo_context *sctx, uint32_t *version, char **commit_id, uint32_t **caps, uint32_t *caps_count)
{
	uint8_t *out;
	uint32_t msg[2];
	uint32_t cmd, out_len;
	uint32_t i;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_GET_CAPABILITY;

	msg[0] = htobe32(2);	// number of words
	msg[1] = htobe32(cmd);

	out_len = 23 * 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 2 * 4, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 23 * 4) {
		free(out);
		return EPROTO;
	}

	*version = be32toh(*(uint32_t *) &out[0]);

	*commit_id = malloc(9);
	if (! *commit_id) {
		free(out);
		return ENOMEM;
	}
	memcpy(*commit_id, &out[4], 8);
	(*commit_id)[8] = '\0';

	*caps = malloc(20 * 4);
	if (! *caps) {
		free(out);
		return ENOMEM;
	}

	*caps_count = 20;
	for (i=0; i<20; i++)
		(*caps)[i] = be32toh(*(uint32_t *) &out[12+i*4]);

	free(out);
	return 0;
}

int sbefifo_get_frequencies(struct sbefifo_context *sctx, uint32_t **freq, uint32_t *freq_count)
{
	uint8_t *out;
	uint32_t msg[2];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_GET_FREQUENCY;

	msg[0] = htobe32(2);	// number of words
	msg[1] = htobe32(cmd);

	out_len = 8 * 4;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 2 * 4, &out, &out_len);
	if (rc)
		return rc;

	*freq_count = out_len / 4;
	if (*freq_count > 0) {
		uint32_t i;

		*freq = malloc(*freq_count * 4);
		if (! *freq) {
			free(out);
			return ENOMEM;
		}

		for (i=0; i<*freq_count; i++)
			(*freq)[i] = be32toh(*(uint32_t *) &out[i*4]);

		free(out);
	} else {
		*freq = NULL;
	}

	return 0;
}

int sbefifo_quiesce(struct sbefifo_context *sctx)
{
	uint8_t *out;
	uint32_t msg[2];
	uint32_t cmd, out_len;
	int rc;

	cmd = SBEFIFO_CMD_CLASS_GENERIC | SBEFIFO_CMD_GET_FREQUENCY;

	msg[0] = htobe32(2);	// number of words
	msg[1] = htobe32(cmd);

	out_len = 0;
	rc = sbefifo_operation(sctx, (uint8_t *)msg, 2 * 4, &out, &out_len);
	if (rc)
		return rc;

	if (out_len != 0) {
		free(out);
		return EPROTO;
	}

	return 0;
}
