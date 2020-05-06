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
#include <inttypes.h>

#include <libpdbg.h>

static void usage(void)
{
	fprintf(stderr, "Usage: libpdbg_attr_test <path> read <prop> array 1|2|4|8 <count>\n");
	fprintf(stderr, "       libpdbg_attr_test <path> write <prop> array 1|2|4|8 <count> <value1> [<value2> ...]\n");
	fprintf(stderr, "       libpdbg_attr_test <path> read <prop> packed <spec> <count>\n");
	fprintf(stderr, "       libpdbg_attr_test <path> write <prop> packed <spec> <count> <value1> [<value2> ...]\n");
	exit(1);
}

static void read_array(struct pdbg_target *target,
		       const char *attr,
		       unsigned int size,
		       unsigned int count)
{
	void *buf;
	unsigned int i;

	buf = malloc(size * count);
	assert(buf);

	if (!pdbg_target_get_attribute(target, attr, size, count, buf))
		exit(88);

	if (size == 1) {
		uint8_t *v = (uint8_t *)buf;

		for (i=0; i<count; i++)
			printf("0x%02x ", v[i]);

	} else if (size == 2) {
		uint16_t *v = (uint16_t *)buf;

		for (i=0; i<count; i++)
			printf("0x%04x ", v[i]);

	} else if (size == 4) {
		uint32_t *v = (uint32_t *)buf;

		for (i=0; i<count; i++)
			printf("0x%08x ", v[i]);

	} else if (size == 8) {
		uint64_t *v = (uint64_t *)buf;

		for (i=0; i<count; i++)
			printf("0x%016" PRIx64 " ", v[i]);

	}
	printf("\n");

	free(buf);
}

static void write_array(struct pdbg_target *target,
			const char *attr,
			unsigned int size,
			unsigned int count,
			const char **argv)
{
	void *buf;
	unsigned int i;

	buf = malloc(size * count);
	assert(buf);

	if (size == 1) {
		uint8_t *v = (uint8_t *)buf;

		for (i=0; i<count; i++)
			v[i] = strtoul(argv[i], NULL, 0) & 0xff;

	} else if (size == 2) {
		uint16_t *v = (uint16_t *)buf;

		for (i=0; i<count; i++)
			v[i] = strtoul(argv[i], NULL, 0) & 0xffff;

	} else if (size == 4) {
		uint32_t *v = (uint32_t *)buf;

		for (i=0; i<count; i++)
			v[i] = strtoul(argv[i], NULL, 0);

	} else if (size == 8) {
		uint64_t *v = (uint64_t *)buf;

		for (i=0; i<count; i++)
			v[i] = strtoull(argv[i], NULL, 0);
	}

	if (!pdbg_target_set_attribute(target, attr, size, count, buf))
		exit(99);

	free(buf);
}

static size_t spec_size(const char *spec)
{
	size_t size = 0, i;

	for (i = 0; i < strlen(spec); i++) {
		char ch = spec[i];

		if (ch == '1')
			size += 1;
		else if (ch == '2')
			size += 2;
		else if (ch == '4')
			size += 4;
		else if (ch == '8')
			size += 8;
		else
			return -1;
	}

	return size;
}

static void read_packed(struct pdbg_target *target,
			const char *attr,
			const char *spec,
			unsigned int count)
{
	void *buf;
	size_t size, pos;
	unsigned int i, j;

	size = spec_size(spec);
	assert(size > 0);

	size = size * count;

	buf = malloc(size);
	assert(buf);

	if (!pdbg_target_get_attribute_packed(target, attr, spec, count, buf))
		exit(88);

	pos = 0;
	for (j=0; j<count; j++) {
		for (i=0; i<strlen(spec); i++) {
			char ch = spec[i];

			if (ch == '1') {
				uint8_t u8 = *(uint8_t *)((uint8_t *)buf + pos);

				printf("0x%02x ", u8);
				pos += 1;

			} else if (ch == '2') {
				uint16_t u16 = *(uint16_t *)((uint8_t *)buf + pos);

				printf("0x%04x ", u16);
				pos += 2;

			} else if (ch == '4') {
				uint32_t u32 = *(uint32_t *)((uint8_t *)buf + pos);

				printf("0x%08x ", u32);
				pos += 4;

			} else if (ch == '8') {
				uint64_t u64 = *(uint64_t *)((uint8_t *)buf + pos);

				printf("0x%016" PRIx64 " ", u64);
				pos += 8;
			}
		}
	}
	printf("\n");

	free(buf);
}

static void write_packed(struct pdbg_target *target,
			 const char *attr,
			 const char *spec,
			 unsigned int count,
			 const char **argv)
{
	void *buf;
	size_t size, pos;
	unsigned int i, j;

	size = spec_size(spec);
	assert(size > 0);

	size = size * count;
	buf = malloc(size);
	assert(buf);

	pos = 0;
	for (j=0; j<count; j++) {
		for (i=0; i<strlen(spec); i++) {
			unsigned index = j * (count-1) + i;
			char ch = spec[i];

			if (ch == '1') {
				uint8_t *v = (uint8_t *)((uint8_t *)buf + pos);

				*v = strtoul(argv[index], NULL, 0) & 0xff;
				pos += 1;

			} else if (ch == '2') {
				uint16_t *v = (uint16_t *)((uint8_t *)buf + pos);

				*v = strtoul(argv[index], NULL, 0) & 0xffffff;
				pos += 2;

			} else if (ch == '4') {
				uint32_t *v = (uint32_t *)((uint8_t *)buf + pos);

				*v = strtoul(argv[index], NULL, 0);
				pos += 4;

			} else if (ch == '8') {
				uint64_t *v = (uint64_t *)((uint8_t *)buf + pos);

				*v = strtoull(argv[index], NULL, 0);
				pos += 8;
			}
		}
	}

	if (!pdbg_target_set_attribute_packed(target, attr, spec, count, buf))
		exit(99);

	free(buf);
}

int main(int argc, const char **argv)
{
	struct pdbg_target *target;
	const char *path, *attr, *spec = NULL;
	bool do_read = false, do_write = false;
	bool is_array = false;
	unsigned int size = 0, count = 0;

	if (argc < 6)
		usage();

	path = argv[1];

	if (strcmp(argv[2], "read") == 0)
		do_read = true;
	else if (strcmp(argv[2], "write") == 0)
		do_write = true;
	else
		usage();

	attr = argv[3];

	if (strcmp(argv[4], "array") == 0)
		is_array = true;
	else if (strcmp(argv[4], "packed") == 0)
		is_array = false;
	else
		usage();

	if (is_array) {
		if (argc < 7)
			usage();

		size = atol(argv[5]);
		if (size != 1 && size != 2 && size != 4 & size != 8)
			usage();

		count = atol(argv[6]);

		if (do_read && argc != 7)
			usage();

		if (do_write && argc != 7 + count)
			usage();
	} else {
		spec = argv[5];
		count = atol(argv[6]);

		if (do_read && argc != 7)
			usage();

		if (do_write && argc != 7 + count * strlen(spec))
			usage();
	}

	pdbg_set_backend(PDBG_BACKEND_FAKE, NULL);
	assert(pdbg_targets_init(NULL));

	target = pdbg_target_from_path(NULL, path);
	if (!target)
		exit(1);

	if (do_read) {
		if (is_array)
			read_array(target, attr, size, count);
		else
			read_packed(target, attr, spec, count);
	}

	if (do_write) {
		if (is_array)
			write_array(target, attr, size, count, &argv[7]);
		else
			write_packed(target, attr, spec, count, &argv[7]);
	}

	return 0;
}
