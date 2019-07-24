/* Copyright 2017 IBM Corp.
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "main.h"
#include "optcmd.h"
#include "path.h"

static int getcfam(uint32_t addr)
{
	struct pdbg_target *target;
	uint32_t value;
	int count = 0;

	for_each_path_target_class("fsi", target) {
		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (fsi_read(target, addr, &value)) {
			printf("p%d: failed\n", pdbg_target_index(target));
			continue;

		}

		printf("p%d: 0x%x = 0x%08x\n", pdbg_target_index(target), addr, value);
		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getcfam, getcfam, (ADDRESS32));

static int putcfam(uint32_t addr, uint32_t data, uint32_t mask)
{
	struct pdbg_target *target;
	int count = 0;

	for_each_path_target_class("fsi", target) {
		int rc;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (mask == 0xffffffff)
			rc = fsi_write(target, addr, data);
		else
			rc = fsi_write_mask(target, addr, data, mask);

		if (rc) {
			printf("p%d: failed\n", pdbg_target_index(target));
			continue;
		}

		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putcfam, putcfam, (ADDRESS32, DATA32, DEFAULT_DATA32("0xffffffff")));
