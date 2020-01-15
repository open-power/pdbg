/* Copyright 2020 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
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
#include <endian.h>
#include <string.h>
#include <assert.h>

#include <libpdbg.h>

static void usage(void)
{
	fprintf(stderr, "Usage: libpdbg_fdt_test <path> read <prop> char|int\n");
	fprintf(stderr, "       libpdbg_fdt_test <path> write <prop> char|int <value>\n");
	exit(1);
}

int main(int argc, const char **argv)
{
	struct pdbg_target *target;
	const char *path, *prop, *prop_value = NULL;
	bool do_read = false, do_write = false;
	bool is_int;

	if (argc < 5)
		usage();

	path = argv[1];

	if (strcmp(argv[2], "read") == 0)
		do_read = true;
	else if (strcmp(argv[2], "write") == 0)
		do_write = true;
	else
		usage();

	prop = argv[3];

	if (strcmp(argv[4], "int") == 0)
		is_int = true;
	else if (strcmp(argv[4], "char") == 0)
		is_int = false;
	else
		usage();

	if (do_read && argc != 5)
		usage();

	if (do_write && argc != 6)
		usage();
	else
		prop_value = argv[5];


	pdbg_set_backend(PDBG_BACKEND_FAKE, NULL);
	assert(pdbg_targets_init(NULL));

	target = pdbg_target_from_path(NULL, path);
	if (!target)
		exit(1);

	if (do_read) {
		if (is_int) {
			uint32_t value;
			int ret;

			ret = pdbg_target_u32_property(target, prop, &value);
			if (ret)
				exit(88);

			printf("0x%08x\n", value);
		} else {
			const void *buf;
			size_t buflen;

			buf = pdbg_target_property(target, prop, &buflen);
			if (!buf)
				exit(88);

			printf("%s<%zu>\n", (const char *)buf, buflen);
		}

	}

	if (do_write) {
		bool ok;

		if (is_int) {
			uint32_t value;

			value = strtoul(prop_value, NULL, 0);
			value = htobe32(value);

			ok = pdbg_target_set_property(target, prop, &value, 4);
		} else {
			size_t len = strlen(prop_value) + 1;

			ok = pdbg_target_set_property(target, prop, prop_value, len);
		}

		if (!ok)
			exit(99);
	}

	return 0;
}
