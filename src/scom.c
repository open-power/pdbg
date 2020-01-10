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
#include <assert.h>

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"
#include "path.h"

/* Check if a target has scom region */
static bool scommable(struct pdbg_target *target)
{
	const char *classname;

	classname = pdbg_target_class_name(target);
	if (!strcmp(classname, "pib") ||
	    !strcmp(classname, "core") ||
	    !strcmp(classname, "thread"))
		return true;

	return false;
}

int getscom(uint64_t addr)
{
	struct pdbg_target *target;
	const char *path;
	uint64_t value;
	int count = 0;

	for_each_path_target(target) {
		struct pdbg_target *addr_base;
		uint64_t xlate_addr;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!scommable(target)) {
			continue;
		}

		path = pdbg_target_path(target);
		xlate_addr = addr;
		addr_base = pdbg_address_absolute(target, &xlate_addr);

		if (pib_read(target, addr, &value)) {
			printf("p%d: 0x%016" PRIx64 " failed (%s)\n", pdbg_target_index(addr_base), xlate_addr, path);
			continue;
		}

		printf("p%d: 0x%016" PRIx64 " = 0x%016" PRIx64 " (%s)\n", pdbg_target_index(addr_base), xlate_addr, value, path);
		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getscom, getscom, (ADDRESS));

int putscom(uint64_t addr, uint64_t data, uint64_t mask)
{
	struct pdbg_target *target;
	const char *path;
	int count = 0;

	for_each_path_target(target) {
		struct pdbg_target *addr_base;
		uint64_t xlate_addr;
		int rc;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!scommable(target)) {
			continue;
		}

		path = pdbg_target_path(target);
		xlate_addr = addr;
		addr_base = pdbg_address_absolute(target, &xlate_addr);

		if (mask == 0xffffffffffffffffULL)
			rc = pib_write(target, addr, data);
		else
			rc = pib_write_mask(target, addr, data, mask);

		if (rc) {
			printf("p%d: 0x%016" PRIx64 " failed (%s)\n", pdbg_target_index(addr_base), xlate_addr, path);
			continue;
		}

		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putscom, putscom, (ADDRESS, DATA, DEFAULT_DATA("0xffffffffffffffff")));
