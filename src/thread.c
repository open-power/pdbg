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
#include <assert.h>
#include <endian.h>

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"
#include "path.h"

static bool is_real_address(struct thread_regs *regs, uint64_t addr)
{
	return true;
	if ((addr & 0xf000000000000000ULL) == 0xc000000000000000ULL)
		return true;
	return false;
}

static int load8(struct pdbg_target *target, uint64_t addr, uint64_t *value)
{
	if (mem_read(target, addr, (uint8_t *)value, 8, 0, false)) {
		pdbg_log(PDBG_ERROR, "Unable to read memory address=%016" PRIx64 ".\n", addr);
		return 0;
	}

	return 1;
}

uint64_t flip_endian(uint64_t v)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return be64toh(v);
#else
	return le64toh(v);
#endif
}

static int dump_stack(struct thread_regs *regs, struct pdbg_target *adu)
{
	uint64_t next_sp = regs->gprs[1];
	uint64_t pc;
	bool finished = false;
	bool prev_flip = false;

	printf("STACK:           SP                NIA\n");
	if (!(next_sp && is_real_address(regs, next_sp))) {
		printf("SP:0x%016" PRIx64 " does not appear to be a stack\n", next_sp);
		return 0;
	}

	while (!finished) {
		uint64_t sp = next_sp;
		uint64_t tmp, tmp2;
		bool flip = false;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		bool be = false;
#else
		bool be = true;
#endif

		if (!is_real_address(regs, sp))
			break;

		if (!load8(adu, sp, &tmp))
			return 1;
		if (!load8(adu, sp + 16, &pc))
			return 1;

		if (!tmp) {
			finished = true;
			flip = prev_flip;
			if (flip)
				be = !be;
			next_sp = 0;
			goto do_pc;
		}

		tmp2 = flip_endian(tmp);

		/*
		 * Basic endian detection.
		 * Stack grows down, so as we unwind it we expect to see
		 * increasing addresses without huge jumps.  The stack may
		 * switch endian-ness across frames in some cases (e.g., LE
		 * kernel calling BE OPAL).
		 */

		/* Check for OPAL stack -> Linux stack */
		if ((sp >= 0x30000000UL && sp < 0x40000000UL) &&
				!(tmp >= 0x30000000UL && tmp < 0x40000000UL)) {
			if (tmp >> 60 == 0xc)
				goto no_flip;
			if (tmp2 >> 60 == 0xc)
				goto do_flip;
		}

		/* Check for Linux -> userspace */
		if ((sp >> 60 == 0xc) && !(tmp >> 60 == 0xc)) {
			finished = true; /* Don't decode userspace */
			if (tmp >> 60 == 0)
				goto no_flip;
			if (tmp2 >> 60 == 0)
				goto do_flip;
		}

		/* Otherwise try to ensure sane stack */
		if (tmp < sp || (tmp - sp > 0xffffffffUL)) {
			if (tmp2 < sp || (tmp2 - sp > 0xffffffffUL)) {
				finished = true;
				goto no_flip;
			}
do_flip:
			next_sp = tmp2;
			flip = true;
			be = !be;
		} else {
no_flip:
			next_sp = tmp;
		}

do_pc:
		if (flip)
			pc = flip_endian(pc);

		printf(" 0x%016" PRIx64 " 0x%016" PRIx64 " (%s)\n",
			sp, pc, be ? "big-endian" : "little-endian");

		prev_flip = flip;
	}
	printf(" 0x%016" PRIx64 "\n", next_sp);

	return 0;
}

static int thr_start(void)
{
	struct pdbg_target *target;
	int count = 0;

	if (path_target_all_selected("thread", NULL)) {
		thread_start_all();
		return 1;
	}

	for_each_path_target_class("thread", target) {
		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		thread_start(target);
		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD(start, thr_start);

static int thr_step(uint64_t steps)
{
	struct pdbg_target *target;
	int count = 0;

	if (path_target_all_selected("thread", NULL)) {
		int i;

		for (i=0; i<count; i++)
			thread_step_all();

		return 1;
	}

	for_each_path_target_class("thread", target) {
		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		thread_step(target, (int)steps);
		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(step, thr_step, (DATA));

static int thr_stop(void)
{
	struct pdbg_target *target;
	int count = 0;

	if (path_target_all_selected("thread", NULL)) {
		thread_stop_all();
		return 1;
	}

	for_each_path_target_class("thread", target) {
		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		thread_stop(target);
		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD(stop, thr_stop);


static int print_one_thread(struct pdbg_target *thread)
{
	struct thread_state tstate;
	char c;

	if (!path_target_selected(thread)) {
		printf("... ");
		return 0;
	}

	tstate = thread_status(thread);


	switch (tstate.sleep_state) {
	case PDBG_THREAD_STATE_DOZE:
		c = 'D';
		break;

	case PDBG_THREAD_STATE_NAP:
		c = 'N';
		break;

	case PDBG_THREAD_STATE_SLEEP:
		c = 'Z';
		break;

	case PDBG_THREAD_STATE_STOP:
		c = 'S';
		break;

	default:
		c = '.';
		break;
	}

	printf("%c%c%c ",
	       (tstate.active ? 'A' : '.'),
	       c,
	       (tstate.quiesced ? 'Q': '.'));
	return 1;
}

static int thread_status_print(void)
{
	struct pdbg_target *thread, *core, *pib;
	int threads_per_core = 0;
	int count = 0;

	for_each_path_target_class("thread", thread) {
		core = pdbg_target_parent("core", thread);
		assert(path_target_add(core));

		pib = pdbg_target_parent("pib", core);
		assert(path_target_add(pib));
	}

	pib = __pdbg_next_target("pib", pdbg_target_root(), NULL, true);
	assert(pib);

	core = __pdbg_next_target("core", pib, NULL, true);
	assert(core);

	pdbg_for_each_target("thread", core, thread)
		threads_per_core++;

	for_each_path_target_class("pib", pib) {
		int i;

		if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
			continue;

		printf("\np%01dt:", pdbg_target_index(pib));
		for (i = 0; i < threads_per_core; i++)
			printf("   %d", i);
		printf("\n");

		pdbg_for_each_target("core", pib, core) {
			if (!path_target_selected(core))
				continue;
			if (pdbg_target_status(core) != PDBG_TARGET_ENABLED)
				continue;

			printf("c%02d:  ", pdbg_target_index(core));

			pdbg_for_each_target("thread", core, thread)
				count += print_one_thread(thread);
			printf("\n");
		}
	}

	return count;
}
OPTCMD_DEFINE_CMD(threadstatus, thread_status_print);

static int thr_sreset(void)
{
	struct pdbg_target *target;
	int count = 0;

	if (path_target_all_selected("thread", NULL)) {
		thread_sreset_all();
		return 1;
	}

	for_each_path_target_class("thread", target) {
		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		thread_sreset(target);
		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD(sreset, thr_sreset);


struct reg_flags {
	bool do_backtrace;
};

#define REG_BACKTRACE_FLAG ("--backtrace", do_backtrace, parse_flag_noarg, false)

static int thread_regs_print(struct reg_flags flags)
{
	struct pdbg_target *pib, *core, *thread;
	struct thread_regs regs;
	int count = 0;

	for_each_path_target_class("thread", thread) {
		core = pdbg_target_parent("core", thread);
		pib = pdbg_target_parent("pib", core);

		printf("p%d c%d t%d\n",
		       pdbg_target_index(pib),
		       pdbg_target_index(core),
		       pdbg_target_index(thread));

		if (thread_getregs(thread, &regs))
			continue;

		if (flags.do_backtrace) {
			struct pdbg_target *adu;

			pdbg_for_each_class_target("mem", adu) {
				if (pdbg_target_probe(adu) == PDBG_TARGET_ENABLED) {
					dump_stack(&regs, adu);
					break;
				}
			}
		}

		count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_ONLY_FLAGS(regs, thread_regs_print, reg_flags, (REG_BACKTRACE_FLAG));
