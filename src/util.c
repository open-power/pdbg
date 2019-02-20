/* Copyright 2018 IBM Corp.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>
#include <inttypes.h>

#include "util.h"

/* Parse argument of the form 0-5,7,9-11,15,17 */
bool parse_list(const char *arg, int max, int *list, int *count)
{
	char str[strlen(arg)+1];
	char *tok, *tmp, *saveptr = NULL;
	int i;

	assert(max < INT_MAX);

	strcpy(str, arg);

	tmp = str;
	while ((tok = strtok_r(tmp, ",", &saveptr)) != NULL) {
		char *a, *b, *endptr, *saveptr2 = NULL;
		unsigned long int from, to;

		a = strtok_r(tok, "-", &saveptr2);
		if (a == NULL) {
			return false;
		} else {
			endptr = NULL;
			from = strtoul(a, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "Invalid value %s\n", a);
				return false;
			}
			if (from >= max) {
				fprintf(stderr, "Value %s larger than max %d\n", a, max-1);
				return false;
			}
		}

		b = strtok_r(NULL, "-", &saveptr2);
		if (b == NULL) {
			to = from;
		} else {
			endptr = NULL;
			to = strtoul(b, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "Invalid value %s\n", b);
				return false;
			}
			if (to >= max) {
				fprintf(stderr, "Value %s larger than max %d\n", b, max-1);
				return false;
			}
		}

		if (from > to) {
			fprintf(stderr, "Invalid range %s-%s\n", a, b);
			return false;
		}

		for (i = from; i <= to; i++)
			list[i] = 1;

		tmp = NULL;
	};

	if (count != NULL) {
		int n = 0;

		for (i = 0; i < max; i++) {
			if (list[i] == 1)
				n++;
		}

		*count = n;
	}

	return true;
}

void hexdump(uint64_t addr, uint8_t *buf, uint64_t size, uint8_t group_size)
{
	uint64_t start_addr, offset, i;
	int j, k;

	start_addr = addr & (~(uint64_t)0xf);
	offset = addr - start_addr;

	if (group_size == 0)
		group_size = 1;

	assert(group_size == 1 || group_size == 2 || group_size == 4 || group_size == 8);

	for (i = 0; i < size + 15; i += 16) {
		bool do_prefix = true;

		if (start_addr + i >= addr + size)
			break;

		for (j = 0; j < 16; j += group_size) {
			for (k = j; k < j + group_size; k++) {
				uint64_t cur_addr = start_addr + i + k;

				if (cur_addr >= addr + size) {
					printf("\n");
					return;
				}

				if (do_prefix) {
					printf("0x%016" PRIx64 ": ", start_addr + i);
					do_prefix = false;
				}
				if (i+k >= offset && i+k <= offset + size)
					printf("%02x", buf[i+k - offset]);
				else
					printf("  ");
			}
			printf(" ");
		}
		printf("\n");
	}
}
