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
#define BLOCK_SIZE (parse_number8_pow2, NULL)

static int _getmem(uint64_t addr, uint64_t size, uint8_t block_size, bool ci)
{
	struct pdbg_target *target;
	uint8_t *buf;
	int rc = 0;

	if (size == 0) {
		PR_ERROR("Size must be > 0\n");
		return 1;
	}

	buf = malloc(size);
	assert(buf);
	pdbg_for_each_class_target("adu", target) {
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			continue;

		pdbg_set_progress_tick(progress_tick);
		progress_init();
		rc = mem_read(target, addr, buf, size, block_size, ci);
		if (rc)
			PR_ERROR("Unable to read memory.\n");

		if (write(STDOUT_FILENO, buf, size) < 0)
			PR_ERROR("Unable to write stdout.\n");
		else
				rc++;

		progress_end();
		break;
	}
	free(buf);
	return rc;

}

static int getmem(uint64_t addr, uint64_t size, struct mem_flags flags)
{
	if (flags.ci)
		return _getmem(addr, size, 8, true);
	else
		return _getmem(addr, size, 0, false);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(getmem, getmem, (ADDRESS, DATA),
			     mem_flags, (MEM_CI_FLAG));

static int getmemio(uint64_t addr, uint64_t size, uint8_t block_size)
{
	return _getmem(addr, size, block_size, true);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getmemio, getmemio, (ADDRESS, DATA, BLOCK_SIZE));

static int _putmem(uint64_t addr, uint8_t block_size, bool ci)
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

		rc = mem_write(adu_target, addr, buf, read_size, block_size, ci);
		if (rc) {
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

static int putmem(uint64_t addr, struct mem_flags flags)
{
	if (flags.ci)
		return _putmem(addr, 8, true);
	else
		return _putmem(addr, 0, false);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(putmem, putmem, (ADDRESS), mem_flags, (MEM_CI_FLAG));

static int putmemio(uint64_t addr, uint8_t block_size)
{
	return _putmem(addr, block_size, true);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putmemio, putmemio, (ADDRESS, BLOCK_SIZE));
