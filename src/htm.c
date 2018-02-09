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
#define _GNU_SOURCE
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <target.h>
#include <operations.h>

#define HTM_DUMP_BASENAME "htm.dump"

static char *get_htm_dump_filename(void)
{
	char *filename;
	int i;

	filename = strdup(HTM_DUMP_BASENAME);
	if (!filename)
		return NULL;

	i = 0;
	while (access(filename, F_OK) == 0) {
		free(filename);
		if (asprintf(&filename, "%s.%d", HTM_DUMP_BASENAME, i) == -1)
			return NULL;
		i++;
	}

	return filename;
}

int run_htm_start(void)
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("Starting HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_start(target) != 1)
			printf("Couldn't start HTM@%" PRIu64 "#%d\n", chip_id, index);
		rc++;
	}

	return rc;
}

int run_htm_stop(void)
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("Stopping HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_stop(target) != 1)
			printf("Couldn't stop HTM@%" PRIu64 "#%d\n", chip_id, index);
		rc++;
	}

	return rc;
}

int run_htm_status(void)
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_status(target) != 1)
			printf("Couldn't get HTM@%" PRIu64 "#%d status\n", chip_id, index);
		rc++;
		printf("\n\n");
	}

	return rc;
}

int run_htm_reset(void)
{
	uint64_t old_base = 0, base, size;
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("Resetting HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_reset(target, &base, &size) != 1)
			printf("Couldn't reset HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (old_base != base) {
			printf("The kernel has initialised HTM memory at:\n");
			printf("base: 0x%016" PRIx64 " for 0x%016" PRIx64 " size\n",
				base, size);
			printf("In case of system crash/xstop use the following to dump the trace on the BMC:\n");
			printf("./pdbg getmem 0x%016" PRIx64 " 0x%016" PRIx64 " > htm.dump\n",
					base, size);
		}
		rc++;
	}

	return rc;
}

int run_htm_dump(void)
{
	struct pdbg_target *target;
	char *filename;
	int rc = 0;

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	/* size = 0 will dump everything */
	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("Dumping HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_dump(target, 0, filename) == 1)
			printf("Couldn't dump HTM@%" PRIu64 "#%d\n", chip_id, index);
		rc++;
	}
	free(filename);

	return rc;
}

int run_htm_trace(void)
{
	uint64_t old_base = 0, base, size;
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		/*
		 * Don't mind if stop fails, it will fail if it wasn't
		 * running, if anything bad is happening reset will fail
		 */
		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		htm_stop(target);
		printf("Resetting HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_reset(target, &base, &size) != 1)
			printf("Couldn't reset HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (old_base != base) {
			printf("The kernel has initialised HTM memory at:\n");
			printf("base: 0x%016" PRIx64 " for 0x%016" PRIx64 " size\n",
					base, size);
			printf("./pdbg getmem 0x%016" PRIx64 " 0x%016" PRIx64 " > htm.dump\n\n",
					base, size);
		}
		old_base = base;
	}

	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("Starting HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_start(target) != 1)
			printf("Couldn't start HTM@%" PRIu64 "#%d\n", chip_id, index);
		rc++;
	}

	return rc;
}

int run_htm_analyse(void)
{
	struct pdbg_target *target;
	char *filename;
	int rc = 0;

	pdbg_for_each_class_target("htm", target)
		htm_stop(target);

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	pdbg_for_each_class_target("htm", target) {
		uint32_t index = pdbg_target_index(target);
		uint64_t chip_id;

		assert(!pdbg_get_u64_property(target, "chip-id", &chip_id));
		printf("Dumping HTM@%" PRIu64 "#%d\n", chip_id, index);
		if (htm_dump(target, 0, filename) != 1)
			printf("Couldn't dump HTM@%" PRIu64 "#%d\n", chip_id, index);
		rc++;
	}
	free(filename);

	return rc;
}
