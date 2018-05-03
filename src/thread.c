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
	status[index] = thread_status(target);
	return 1;
}

static int print_core_thread_status(struct pdbg_target *core_target, uint32_t index, uint64_t *maxindex, uint64_t *unused1)
{
	uint64_t status[8];
	int i, rc;

	memset(status, 0xff, sizeof(status));

	printf("c%02d: ", index);
	rc = for_each_child_target("thread", core_target, print_thread_status, &status[0], NULL);
	for (i = 0; i <= *maxindex; i++) {
		if (status[i] == -1ULL) {
			printf("    ");
			continue;
		}
		if (status[i] & ~(THREAD_STATUS_ACTIVE|THREAD_STATUS_DOZE|
				  THREAD_STATUS_NAP|THREAD_STATUS_SLEEP|
				  THREAD_STATUS_STOP|THREAD_STATUS_QUIESCE)) {
			printf("%" PRIx64 " ", status[i]);
			continue;
		}

		if (status[i] & THREAD_STATUS_ACTIVE)
			printf("A");
		else
			printf(".");

		if (status[i] & THREAD_STATUS_DOZE)
			printf("D");
		else if (status[i] & THREAD_STATUS_NAP)
			printf("N");
		else if (status[i] & THREAD_STATUS_SLEEP)
			printf("S");
		else if (status[i] & THREAD_STATUS_STOP)
			printf("S");
		else
			printf(".");

		if (status[i] & THREAD_STATUS_QUIESCE)
			printf("Q");
		else
			printf(".");
		printf(" ");

	}
	printf("\n");

	return rc;
}

static int get_thread_max_index(struct pdbg_target *target, uint32_t index, uint64_t *maxindex, uint64_t *unused)
{
	if (index > *maxindex)
		*maxindex = index;
	return 1;
}

static int get_core_max_threads(struct pdbg_target *core_target, uint32_t index, uint64_t *maxindex, uint64_t *unused1)
{
	return for_each_child_target("thread", core_target, get_thread_max_index, maxindex, NULL);
}

static int print_proc_thread_status(struct pdbg_target *pib_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	int i;
	uint64_t maxindex = 0;

	for_each_child_target("core", pib_target, get_core_max_threads, &maxindex, NULL);

	printf("\np%01dt:", index);
	for (i = 0; i <= maxindex; i++)
		printf("   %d", i);
	printf("\n");

	return for_each_child_target("core", pib_target, print_core_thread_status, &maxindex, NULL);
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

static int state_thread(struct pdbg_target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	struct thread_regs regs;

	return ram_state_thread(thread_target, &regs) ? 0 : 1;
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

int thread_state(int optind, int argc, char *argv[])
{
	int err;

	err = for_each_target("thread", state_thread, NULL, NULL);

	for_each_target_release("thread");

	return err;
}
