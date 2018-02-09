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

static int getscom(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *unused)
{
	uint64_t value;

	if (pib_read(target, *addr, &value))
		return 0;

	printf("p%d:0x%" PRIx64 " = 0x%016" PRIx64 "\n", index, *addr, value);

	return 1;
}

static int putscom(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *data)
{
	if (pib_write(target, *addr, *data))
		return 0;

	return 1;
}


int handle_scoms(int optind, int argc, char *argv[])
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

	if (strcmp(argv[optind], "putscom") == 0) {
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

		return for_each_target("pib", putscom, &addr, &data);
	}

	return for_each_target("pib", getscom, &addr, NULL);
}

