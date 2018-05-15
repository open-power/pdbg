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

#include <operations.h>
#include <target.h>

#include "main.h"

#define PUTMEM_BUF_SIZE 1024
static int getmem(uint64_t addr, uint64_t size, int ci)
{
	struct pdbg_target *target;
	uint8_t *buf;
	int rc = 0;
	buf = malloc(size);
	assert(buf);
	pdbg_for_each_class_target("adu", target) {
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!adu_getmem(target, addr, buf, size, ci)) {
			if (write(STDOUT_FILENO, buf, size) < 0)
				PR_ERROR("Unable to write stdout.\n");
			else
				rc++;
		} else
			PR_ERROR("Unable to read memory.\n");
			/* We only ever care about getting memory from a single processor */
		break;
	}
	free(buf);
	return rc;

}
static int putmem(uint64_t addr, int ci)
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
	do {
		read_size = read(STDIN_FILENO, buf, PUTMEM_BUF_SIZE);
		if (adu_putmem(adu_target, addr, buf, read_size, ci)) {
			rc = 0;
			printf("Unable to write memory.\n");
			break;
		}
		rc += read_size;
	} while (read_size > 0);

	printf("Wrote %d bytes starting at 0x%016" PRIx64 "\n", rc, addr);
	free(buf);
	return rc;
}

int handle_mem(int optind, int argc, char *argv[])
{
	uint64_t addr;
	char *endptr;
	int ci = 0;

	if (optind + 1 >= argc) {
		printf("%s: command '%s' requires an address\n", argv[0], argv[optind]);
		return -1;
	}

	errno = 0;

	if (strcmp(argv[optind +1], "-ci") == 0) {
		/* Set cache-inhibited flag */
		ci = 1;
	}

	addr = strtoull(argv[optind + 1 + ci], &endptr, 0);
	if (errno || *endptr != '\0') {
		printf("%s: command '%s' couldn't parse address '%s'\n",
				argv[0], argv[optind], argv[optind + 1 + ci]);
		return -1;
	}

	if (strcmp(argv[optind], "getmem") == 0) {
		uint64_t size;

		if (optind + 2 + ci >= argc) {
			printf("%s: command '%s' requires data\n", argv[0], argv[optind]);
			return -1;
		}

		errno = 0;
		size = strtoull(argv[optind + 2 + ci], &endptr, 0);
		if (errno || *endptr != '\0') {
			printf("%s: command '%s' couldn't parse data '%s'\n",
				argv[0], argv[optind], argv[optind + 1 + ci]);
			return -1;
		}

		return getmem(addr, size, ci);
	}

	return putmem(addr, ci);
}
