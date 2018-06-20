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

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"

static int print_thread_status(struct pdbg_target *target, uint32_t index, uint64_t *arg, uint64_t *unused1)
{
	struct thread_state *status = (struct thread_state *) arg;

	status[index] = thread_status(target);
	return 1;
}

static int print_core_thread_status(struct pdbg_target *core_target, uint32_t index, uint64_t *maxindex, uint64_t *unused1)
{
	struct thread_state status[8];
	int i, rc;

	printf("c%02d:  ", index);

	/* TODO: This cast is gross. Need to rewrite for_each_child_target as an iterator. */
	rc = for_each_child_target("thread", core_target, print_thread_status, (uint64_t *) &status[0], NULL);
	for (i = 0; i <= *maxindex; i++) {

		if (status[i].active)
			printf("A");
		else
			printf(".");

		switch (status[i].sleep_state) {
		case PDBG_THREAD_STATE_DOZE:
			printf("D");
			break;

		case PDBG_THREAD_STATE_NAP:
			printf("N");
			break;

		case PDBG_THREAD_STATE_SLEEP:
			printf("Z");
			break;

		case PDBG_THREAD_STATE_STOP:
			printf("S");
			break;

		default:
			printf(".");
			break;
		}

		if (status[i].quiesced)
			printf("Q");
		else
			printf(".");
		printf(" ");

	}
	printf("\n");

	return rc;
}

static bool is_real_address(struct thread_regs *regs, uint64_t addr)
{
	return true;
	if ((addr & 0xf000000000000000ULL) == 0xc000000000000000ULL)
		return true;
	return false;
}

static int load8(struct pdbg_target *target, uint64_t addr, uint64_t *value)
{
	if (adu_getmem(target, addr, (uint8_t *)value, 8)) {
		pdbg_log(PDBG_ERROR, "Unable to read memory address=%016" PRIx64 ".\n", addr);
		return 0;
	}

	return 1;
}

static int dump_stack(struct thread_regs *regs)
{
	struct pdbg_target *target;
	uint64_t sp = regs->gprs[1];
	uint64_t pc;

	pdbg_for_each_class_target("adu", target) {
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			continue;
		break;
	}

	printf("STACK:\n");
	if (!target)
		pdbg_log(PDBG_ERROR, "Unable to read memory (no ADU found)\n");

	if (sp && is_real_address(regs, sp)) {
		if (!load8(target, sp, &sp))
			return 1;
		while (sp && is_real_address(regs, sp)) {
			if (!load8(target, sp + 16, &pc))
				return 1;

			printf(" 0x%016" PRIx64 " 0x%16" PRIx64 "\n", sp, pc);

			if (!load8(target, sp, &sp))
				return 1;
		}
	}

	return 0;
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

	if (ram_state_thread(thread_target, &regs))
		return 0;

	dump_stack(&regs);

	return 1;
}

static int thread_start(void)
{
	return for_each_target("thread", start_thread, NULL, NULL);
}
OPTCMD_DEFINE_CMD(start, thread_start);

static int thread_step(uint64_t count)
{
	return for_each_target("thread", step_thread, &count, NULL);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(step, thread_step, (DATA));

static int thread_stop(void)
{
	return for_each_target("thread", stop_thread, NULL, NULL);
}
OPTCMD_DEFINE_CMD(stop, thread_stop);

static int thread_status_print(void)
{
	return for_each_target("pib", print_proc_thread_status, NULL, NULL);
}
OPTCMD_DEFINE_CMD(threadstatus, thread_status_print);

static int thread_sreset(void)
{
	return for_each_target("thread", sreset_thread, NULL, NULL);
}
OPTCMD_DEFINE_CMD(sreset, thread_sreset);

static int thread_state(void)
{
	int err;

	err = for_each_target("thread", state_thread, NULL, NULL);

	for_each_target_release("thread");

	return err;
}
OPTCMD_DEFINE_CMD(regs, thread_state);
