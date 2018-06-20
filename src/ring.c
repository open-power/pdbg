/* Copyright 2018 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"

static int pdbg_getring(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *len)
{
	uint32_t *result;
	int i, words;
	int ring_len = *len;

	words = (ring_len + 32 - 1)/32;

	result = calloc(words, sizeof(*result));
	assert(result);

	getring(target, *addr, ring_len, result);

	for (i = 0; i < ring_len/32; i++)
		printf("%08" PRIx32, result[i]);

	ring_len -= i*32;

	/* Print out remaining bits */
	for (i = 0; i < (ring_len + 4 - 1)/4; i++)
		printf("%01" PRIx32, (result[words - 1] >> (28 - i*4)) & 0xf);

	printf("\n");

	return 1;
}

static int _getring(uint64_t ring_addr, uint64_t ring_len)
{
	return for_each_target("chiplet", pdbg_getring, &ring_addr, &ring_len);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getring, _getring, (ADDRESS, DATA));
