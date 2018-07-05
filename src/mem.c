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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

#include <libpdbg.h>

#include "main.h"
#include "progress.h"
#include "optcmd.h"
#include "parsers.h"

#define PR_ERROR(x, args...) \
	pdbg_log(PDBG_ERROR, x, ##args)

#define PUTMEM_BUF_SIZE 1024

struct mem_flags {
	bool ci;
};

#define MEM_CI_FLAG ("--ci", ci, parse_flag_noarg, false)

static int getmem(uint64_t addr, uint64_t size, struct mem_flags flags)
{
	struct pdbg_target *target;
	uint8_t *buf;
	int rc = 0;

	buf = malloc(size);
	assert(buf);
	pdbg_for_each_class_target("adu", target) {
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			continue;

		pdbg_set_progress_tick(progress_tick);
		progress_init();
		if (!__adu_getmem(target, addr, buf, size, flags.ci)) {
			if (write(STDOUT_FILENO, buf, size) < 0)
				PR_ERROR("Unable to write stdout.\n");
			else
				rc++;
		} else
			PR_ERROR("Unable to read memory.\n");
			/* We only ever care about getting memory from a single processor */
		progress_end();
		break;
	}
	free(buf);
	return rc;

}
OPTCMD_DEFINE_CMD_WITH_FLAGS(getmem, getmem, (ADDRESS, DATA),
			     mem_flags, (MEM_CI_FLAG));

static int putmem(uint64_t addr, struct mem_flags flags)
{
	uint8_t *buf;
	int read_size, rc = 0;
	struct pdbg_target *adu_target;

	pdbg_for_each_class_target("adu", adu_target)
		break;

	if (pdbg_target_probe(adu_target) != PDBG_TARGET_ENABLED)
		return 0;

	buf = malloc(PUTMEM_BUF_SIZE);
	assert(buf);
	pdbg_set_progress_tick(progress_tick);
	progress_init();
	do {
		read_size = read(STDIN_FILENO, buf, PUTMEM_BUF_SIZE);
		if (read_size <= 0)
			break;

		if (__adu_putmem(adu_target, addr, buf, read_size, flags.ci)) {
			rc = 0;
			printf("Unable to write memory.\n");
			break;
		}
		rc += read_size;
	} while (read_size > 0);
	progress_end();

	printf("Wrote %d bytes starting at 0x%016" PRIx64 "\n", rc, addr);
	free(buf);
	return rc;
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(putmem, putmem, (ADDRESS),
			     mem_flags, (MEM_CI_FLAG));
