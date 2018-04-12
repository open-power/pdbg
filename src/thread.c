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

#include <bitutils.h>

#include <target.h>
#include <operations.h>

#include "main.h"

static int print_thread_status(struct pdbg_target *target, uint32_t index, uint64_t *status, uint64_t *unused1)
{
	*status = SETFIELD(0xffULL << (index * 8), *status, thread_status(target) & 0xffULL);
	return 1;
}

static int print_core_thread_status(struct pdbg_target *core_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	uint64_t status = -1UL;
	int i, rc;

	printf("c%02d:", index);
	rc = for_each_child_target("thread", core_target, print_thread_status, &status, NULL);
	for (i = 0; i < 8; i++) {
		switch ((status >> (i * 8)) & 0xf) {
		case THREAD_STATUS_ACTIVE:
			printf(" A");
			break;

		case THREAD_STATUS_DOZE:
		case THREAD_STATUS_QUIESCE | THREAD_STATUS_DOZE:
			printf(" D");
			break;

		case THREAD_STATUS_NAP:
		case THREAD_STATUS_QUIESCE | THREAD_STATUS_NAP:
			printf(" N");
			break;

		case THREAD_STATUS_SLEEP:
		case THREAD_STATUS_QUIESCE | THREAD_STATUS_SLEEP:
			printf(" S");
			break;

		case THREAD_STATUS_ACTIVE | THREAD_STATUS_QUIESCE:
			printf(" Q");
			break;

		case 0xf:
			printf("  ");
			break;

		default:
			printf(" U");
			break;
		}
	}
	printf("\n");

	return rc;
}

static int print_proc_thread_status(struct pdbg_target *pib_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	printf("\np%01dt: 0 1 2 3 4 5 6 7\n", index);
	return for_each_child_target("core", pib_target, print_core_thread_status, NULL, NULL);
};

static int start_thread(struct pdbg_target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	return ram_start_thread(thread_target) ? 0 : 1;
}

static int step_thread(struct pdbg_target *thread_target, uint32_t index, uint64_t *count, uint64_t *unused1)
{
	return ram_step_thread(thread_target, *count) ? 0 : 1;
}

static int stop_thread(struct pdbg_target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	return ram_stop_thread(thread_target) ? 0 : 1;
}

static int sreset_thread(struct pdbg_target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	return ram_sreset_thread(thread_target) ? 0 : 1;
}

int thread_start(int optind, int argc, char *argv[])
{
	return for_each_target("thread", start_thread, NULL, NULL);
}

int thread_step(int optind, int argc, char *argv[])
{
	uint64_t count;
	char *endptr;

	if (optind + 1 >= argc) {
		printf("%s: command '%s' requires a count\n", argv[0], argv[optind]);
		return -1;
	}

	errno = 0;
	count = strtoull(argv[optind + 1], &endptr, 0);
	if (errno || *endptr != '\0') {
		printf("%s: command '%s' couldn't parse count '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
		return -1;
	}

	return for_each_target("thread", step_thread, &count, NULL);
}

int thread_stop(int optind, int argc, char *argv[])
{
	return for_each_target("thread", stop_thread, NULL, NULL);
}

int thread_status_print(int optind, int argc, char *argv[])
{
	return for_each_target("pib", print_proc_thread_status, NULL, NULL);
}

int thread_sreset(int optind, int argc, char *argv[])
{
	return for_each_target("thread", sreset_thread, NULL, NULL);
}
