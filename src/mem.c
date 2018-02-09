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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <operations.h>
#include <target.h>

#define PUTMEM_BUF_SIZE 1024
int putmem(uint64_t addr)
{
	uint8_t *buf;
	int read_size, rc = 0;
	struct pdbg_target *adu_target;

	pdbg_for_each_class_target("adu", adu_target)
		break;

	buf = malloc(PUTMEM_BUF_SIZE);
	assert(buf);
	do {
		read_size = read(STDIN_FILENO, buf, PUTMEM_BUF_SIZE);
		if (adu_putmem(adu_target, addr, buf, read_size)) {
			rc = 0;
			printf("Unable to write memory.\n");
			break;
		}
		rc += read_size;
	} while (read_size > 0);

	free(buf);
	return rc;
}
