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
#include <ctype.h>

#include <libpdbg.h>

#include "main.h"
#include "progress.h"
#include "optcmd.h"
#include "parsers.h"
#include "util.h"
#include "path.h"

#define PR_ERROR(x, args...) \
	pdbg_log(PDBG_ERROR, x, ##args)

#define PUTMEM_BUF_SIZE 1024

struct mem_flags {
	bool ci;
	bool raw;
};

struct mem_io_flags {
	bool raw;
};

#define MEM_CI_FLAG ("--ci", ci, parse_flag_noarg, false)
#define MEM_RAW_FLAG ("--raw", raw, parse_flag_noarg, false)

#define BLOCK_SIZE (parse_number8_pow2, NULL)

static uint8_t *read_stdin(size_t *size)
{
	uint8_t *buf = NULL;
	size_t allocated = 0;
	size_t buflen = 0;
	ssize_t n;

	while (1) {
		if (allocated == buflen) {
			uint8_t *ptr;

			ptr = realloc(buf, allocated + PUTMEM_BUF_SIZE);
			if (!ptr) {
				free(buf);
				return NULL;
			}
			buf = ptr;
			allocated += PUTMEM_BUF_SIZE;
		}

		n = read(STDIN_FILENO, buf + buflen, allocated - buflen);
		if (n <= 0)
			break;

		buflen += n;
	}

	*size = buflen;
	return buf;
}

static int _getmem(const char *mem_prefix, uint64_t addr, uint64_t size, uint8_t block_size, bool ci, bool raw)
{
	struct pdbg_target *target;
	uint8_t *buf;
	int rc, count = 0;

	if (size == 0) {
		PR_ERROR("Size must be > 0\n");
		return 1;
	}

	buf = malloc(size);
	assert(buf);

	for_each_path_target_class("pib", target) {
		char mem_path[128];
		struct pdbg_target *mem;

		sprintf(mem_path, "/%s%u", mem_prefix, pdbg_target_index(target));

		mem = pdbg_target_from_path(NULL, mem_path);
		if (!mem)
			continue;

		if (pdbg_target_probe(mem) != PDBG_TARGET_ENABLED)
			continue;

		pdbg_set_progress_tick(progress_tick);
		progress_init();
		rc = mem_read(mem, addr, buf, size, block_size, ci);
		progress_end();
		if (rc) {
			PR_ERROR("Unable to read memory from %s\n",
				 pdbg_target_path(mem));
			continue;
		}

		count++;
		break;
	}

	if (count > 0) {
		if (raw) {
			if (write(STDOUT_FILENO, buf, size) < 0)
				PR_ERROR("Unable to write stdout.\n");
		} else {
			hexdump(addr, buf, size, 1);
		}
	}

	free(buf);
	return count;
}

static int getmem(uint64_t addr, uint64_t size, struct mem_flags flags)
{
	if (flags.ci)
		return _getmem("mem", addr, size, 8, true, flags.raw);
	else
		return _getmem("mem", addr, size, 0, false, flags.raw);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(getmem, getmem, (ADDRESS, DATA),
			     mem_flags, (MEM_CI_FLAG, MEM_RAW_FLAG));

static int getmempba(uint64_t addr, uint64_t size, struct mem_flags flags)
{
	if (flags.ci)
		return _getmem("mempba", addr, size, 0, true, flags.raw);
	else
		return _getmem("mempba", addr, size, 0, false, flags.raw);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(getmempba, getmempba, (ADDRESS, DATA),
			     mem_flags, (MEM_CI_FLAG, MEM_RAW_FLAG));

static int getmemio(uint64_t addr, uint64_t size, uint8_t block_size, struct mem_io_flags flags)
{
	return _getmem("mem", addr, size, block_size, true, flags.raw);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(getmemio, getmemio, (ADDRESS, DATA, BLOCK_SIZE),
			     mem_io_flags, (MEM_RAW_FLAG));

static int _putmem(const char *mem_prefix, uint64_t addr, uint8_t block_size, bool ci)
{
	uint8_t *buf;
	size_t buflen = 0;
	int rc, count = 0;
	struct pdbg_target *target;

	buf = read_stdin(&buflen);
	assert(buf);

	for_each_path_target_class("pib", target) {
		char mem_path[128];
		struct pdbg_target *mem;

		sprintf(mem_path, "/%s%u", mem_prefix, pdbg_target_index(target));

		mem = pdbg_target_from_path(NULL, mem_path);
		if (!mem)
			continue;

		if (pdbg_target_probe(mem) != PDBG_TARGET_ENABLED)
			continue;

		pdbg_set_progress_tick(progress_tick);
		progress_init();
		rc = mem_write(mem, addr, buf, buflen, block_size, ci);
		progress_end();
		if (rc) {
			printf("Unable to write memory using %s\n",
			       pdbg_target_path(mem));
			continue;
		}

		count++;
		break;
	}

	if (count > 0)
		printf("Wrote %zu bytes starting at 0x%016" PRIx64 "\n", buflen, addr);

	free(buf);
	return count;
}

static int putmem(uint64_t addr, struct mem_flags flags)
{
	if (flags.ci)
		return _putmem("mem", addr, 8, true);
	else
		return _putmem("mem", addr, 0, false);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(putmem, putmem, (ADDRESS), mem_flags, (MEM_CI_FLAG));

static int putmempba(uint64_t addr, struct mem_flags flags)
{
	if (flags.ci)
		return _putmem("mempba", addr, 0, true);
	else
		return _putmem("mempba", addr, 0, false);
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(putmempba, putmempba, (ADDRESS), mem_flags, (MEM_CI_FLAG));

static int putmemio(uint64_t addr, uint8_t block_size)
{
	return _putmem("mem", addr, block_size, true);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putmemio, putmemio, (ADDRESS, BLOCK_SIZE));
