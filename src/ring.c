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
#include "path.h"

static int get_ring(uint64_t ring_addr, uint64_t ring_len)
{
	struct pdbg_target *target;
	uint32_t *result;
	int count = 0;
	int words;

	words = (ring_len + 32 - 1) / 32;

	result = calloc(words, sizeof(*result));
	assert(result);

	for_each_path_target_class("chiplet", target) {
		char *path;
		int rc, i, len;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		path = pdbg_target_path(target);
		assert(path);

		printf("%s: 0x%016" PRIx64 " = ", path, ring_addr);
		free(path);

		rc = getring(target, ring_addr, ring_len, result);
		if (rc) {
			printf("failed\n");
			continue;
		}

		printf("\n");

		len = (int)ring_len;
		for (i = 0; i < len/32; i++)
			printf("%08" PRIx32, result[i]);

		len -= i*32;

		for (i=0; i < (len + 4 - 1)/4; i++)
			printf("%01" PRIx32, (result[words-1] >> (28 - i*4)) & 0xf);

		printf("\n");

		count++;
	}

	free(result);
	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getring, get_ring, (ADDRESS, DATA));
