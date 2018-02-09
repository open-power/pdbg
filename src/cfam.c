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

#include <target.h>

#include "main.h"

static int getcfam(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *unused)
{
	uint32_t value;

	if (fsi_read(target, *addr, &value))
		return 0;

	printf("p%d:0x%x = 0x%08x\n", index, (uint32_t) *addr, value);

	return 1;
}

static int putcfam(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *data)
{
	if (fsi_write(target, *addr, *data))
		return 0;

	return 1;
}

int handle_cfams(int optind, int argc, char *argv[])
{
	uint64_t addr;
	char *endptr;

	if (optind + 1 >= argc) {
		printf("%s: command '%s' requires an address\n", argv[0], argv[optind]);
		return -1;
	}

	errno = 0;
	addr = strtoull(argv[optind + 1], &endptr, 0);
	if (errno || *endptr != '\0') {
		printf("%s: command '%s' couldn't parse address '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
		return -1;
	}

	if (strcmp(argv[optind], "putcfam") == 0) {
		uint64_t data;

		if (optind + 2 >= argc) {
			printf("%s: command '%s' requires data\n", argv[0], argv[optind]);
			return -1;
		}

		errno = 0;
		data = strtoull(argv[optind + 2], &endptr, 0);
		if (errno || *endptr != '\0') {
			printf("%s: command '%s' couldn't parse data '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
			return -1;
		}

		return for_each_target("fsi", putcfam, &addr, &data);
	}

	return for_each_target("fsi", getcfam, &addr, NULL);
}


