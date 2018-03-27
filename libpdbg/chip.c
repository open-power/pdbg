/* Copyright 2016 IBM Corp.
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
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <ccan/array_size/array_size.h>
#include <unistd.h>

#include "target.h"
#include "operations.h"
#include "bitutils.h"
#include "debug.h"

static uint64_t mfspr(uint64_t reg, uint64_t spr)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");

	return MFSPR_OPCODE | (reg << 21) | ((spr & 0x1f) << 16) | ((spr & 0x3e0) << 6);
}

static uint64_t mtspr(uint64_t spr, uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");

	return MTSPR_OPCODE | (reg << 21) | ((spr & 0x1f) << 16) | ((spr & 0x3e0) << 6);
}

static uint64_t mfocrf(uint64_t reg, uint64_t cr)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");
	if (cr > 7)
		PR_ERROR("Invalid register specified\n");

	return MFOCRF_OPCODE | (reg << 21) | (1U << (12 + cr));
}

static uint64_t mfnia(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");

	return MFNIA_OPCODE | (reg << 21);
}

static uint64_t mtnia(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");

	return MTNIA_OPCODE | (reg << 21);
}

static uint64_t mfmsr(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");

	return MFMSR_OPCODE | (reg << 21);
}

static uint64_t mtmsr(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified\n");

	return MTMSR_OPCODE | (reg << 21);
}

static uint64_t ld(uint64_t rt, uint64_t ds, uint64_t ra)
{
	if ((rt > 31) | (ra > 31) | (ds > 0x3fff))
		PR_ERROR("Invalid register specified\n");

	return LD_OPCODE | (rt << 21) | (ra << 16) | (ds << 2);
}

uint64_t thread_status(struct pdbg_target *target)
{
	struct thread *thread;

	assert(!strcmp(target->class, "thread"));
	thread = target_to_thread(target);
	return thread->status;
}

/*
 * Single step the thread count instructions.
 */
int ram_step_thread(struct pdbg_target *thread_target, int count)
{
	struct thread *thread;

	assert(!strcmp(thread_target->class, "thread"));
	thread = target_to_thread(thread_target);
	return thread->step(thread, count);
}

int ram_start_thread(struct pdbg_target *thread_target)
{
	struct thread *thread;

	assert(!strcmp(thread_target->class, "thread"));
	thread = target_to_thread(thread_target);
	return thread->start(thread);
}

int ram_stop_thread(struct pdbg_target *thread_target)
{
	struct thread *thread;

	assert(!strcmp(thread_target->class, "thread"));
	thread = target_to_thread(thread_target);
	return thread->stop(thread);
}

int ram_sreset_thread(struct pdbg_target *thread_target)
{
	struct thread *thread;

	assert(!strcmp(thread_target->class, "thread"));
	thread = target_to_thread(thread_target);
	return thread->sreset(thread);
}

/*
 * RAMs the opcodes in *opcodes and store the results of each opcode
 * into *results. *results must point to an array the same size as
 * *opcodes. Each entry from *results is put into SCR0 prior to
 * executing an opcode so that it may also be used to pass in
 * data. Note that only register r0 is saved and restored so opcodes
 * must not touch other registers.
 */
static int ram_instructions(struct pdbg_target *thread_target, uint64_t *opcodes,
			    uint64_t *results, int len, unsigned int lpar)
{
	uint64_t opcode = 0, r0 = 0, r1 = 0, scratch = 0;
	int i;
	int exception = 0;
	struct thread *thread;
	bool did_setup = false;

	assert(!strcmp(thread_target->class, "thread"));
	thread = target_to_thread(thread_target);

	if (!thread->ram_is_setup) {
		CHECK_ERR(thread->ram_setup(thread));
		did_setup = true;
	}

	/* RAM instructions */
	for (i = -2; i < len + 2; i++) {
		if (i == -2)
			opcode = mtspr(277, 1);
		else if (i == -1)
			/* Save r0 (assumes opcodes don't touch other registers) */
			opcode = mtspr(277, 0);
		else if (i < len) {
			scratch = results[i];
			opcode = opcodes[i];
		} else if (i == len) {
			/* Restore r0 */
			scratch = r0;
			opcode = mfspr(0, 277);
		} else if (i == len + 1) {
			/* Restore r1 */
			scratch = r1;
			opcode = mfspr(1, 277);
		}

		if (thread->ram_instruction(thread, opcode, &scratch)) {
			PR_DEBUG("%s: %d\n", __FUNCTION__, __LINE__);
			exception = 1;
			break;
		}

		if (i == -2)
			r1 = scratch;
		else if (i == -1)
			r0 = scratch;
		else if (i < len)
			results[i] = scratch;
	}

	if (did_setup)
		CHECK_ERR(thread->ram_destroy(thread));

	return exception;
}

/*
 * Get gpr value. Chip must be stopped.
 */
int ram_getgpr(struct pdbg_target *thread, int gpr, uint64_t *value)
{
	uint64_t opcodes[] = {mtspr(277, gpr)};
	uint64_t results[] = {0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[0];
	return 0;
}

int ram_putgpr(struct pdbg_target *thread, int gpr, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(gpr, 277)};
	uint64_t results[] = {value};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));

	return 0;
}

int ram_getnia(struct pdbg_target *thread, uint64_t *value)
{
	uint64_t opcodes[] = {mfnia(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_putnia(struct pdbg_target *thread, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mtnia(0)};
	uint64_t results[] = {value, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int ram_getspr(struct pdbg_target *thread, int spr, uint64_t *value)
{
	uint64_t opcodes[] = {mfspr(0, spr), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_putspr(struct pdbg_target *thread, int spr, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mtspr(spr, 0)};
	uint64_t results[] = {value, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int ram_getmsr(struct pdbg_target *thread, uint64_t *value)
{
	uint64_t opcodes[] = {mfmsr(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_getcr(struct pdbg_target *thread, int cr, uint64_t *value)
{
	uint64_t opcodes[] = {mfocrf(0, cr), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_putmsr(struct pdbg_target *thread, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mtmsr(0)};
	uint64_t results[] = {value, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int ram_getmem(struct pdbg_target *thread, uint64_t addr, uint64_t *value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mfspr(1, 277), ld(0, 0, 1), mtspr(277, 0)};
	uint64_t results[] = {0xdeaddeaddeaddead, addr, 0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[3];
	return 0;
}

/*
 * Read the given ring from the given chiplet. Result must be large enough to hold ring_len bits.
 */
int getring(struct pdbg_target *chiplet_target, uint64_t ring_addr, uint64_t ring_len, uint32_t result[])
{
	struct chiplet *chiplet;

	assert(!strcmp(chiplet_target->class, "chiplet"));
	chiplet = target_to_chiplet(chiplet_target);
	return chiplet->getring(chiplet, ring_addr, ring_len, result);
}

int ram_state_thread(struct pdbg_target *thread, struct thread_regs *regs)
{
	struct thread_regs _regs;
	struct thread *t;
	uint64_t value;
	int i;

	if (!regs)
		regs = &_regs;

	assert(!strcmp(thread->class, "thread"));
	t = target_to_thread(thread);

	CHECK_ERR(t->ram_setup(t));

	/*
	 * It would be neat to do all the ramming up front, then go through
	 * and print everything out somewhere else. In practice so far it
	 * can help to diagnose checkstop issues with ramming to print as
	 * we go. Once it's more robust and tested, maybe.
	 */
	ram_getnia(thread, &regs->nia);
	printf("NIA   : 0x%016" PRIx64 "\n", regs->nia);

	ram_getspr(thread, 28, &regs->cfar);
	printf("CFAR  : 0x%016" PRIx64 "\n", regs->cfar);

	ram_getmsr(thread, &regs->msr);
	printf("MSR   : 0x%016" PRIx64 "\n", regs->msr);

	ram_getspr(thread, 8, &regs->lr);
	printf("LR    : 0x%016" PRIx64 "\n", regs->lr);

	ram_getspr(thread, 9, &regs->ctr);
	printf("CTR   : 0x%016" PRIx64 "\n", regs->ctr);

	ram_getspr(thread, 815, &regs->tar);
	printf("TAR   : 0x%016" PRIx64 "\n", regs->tar);

	regs->cr = 0;
	for (i = 0; i < 8; i++) {
		uint64_t cr;
		ram_getcr(thread, i, &cr);
		regs->cr |= cr;
	}
	printf("CR    : 0x%08" PRIx32 "\n", regs->cr);

	ram_getspr(thread, 0x1, &value);
	regs->xer = value;
	printf("XER   : 0x%08" PRIx32 "\n", regs->xer);

	printf("GPRS  :\n");
	for (i = 0; i < 32; i++) {
		ram_getgpr(thread, i, &regs->gprs[i]);
		printf(" 0x%016" PRIx64 "", regs->gprs[i]);
		if (i % 4 == 3)
			printf("\n");
	}

	ram_getspr(thread, 318, &regs->lpcr);
	printf("LPCR  : 0x%016" PRIx64 "\n", regs->lpcr);

	ram_getspr(thread, 464, &regs->ptcr);
	printf("PTCR  : 0x%016" PRIx64 "\n", regs->lpcr);

	ram_getspr(thread, 319, &regs->lpidr);
	printf("LPIDR : 0x%016" PRIx64 "\n", regs->lpidr);

	ram_getspr(thread, 48, &regs->pidr);
	printf("PIDR  : 0x%016" PRIx64 "\n", regs->pidr);

	ram_getspr(thread, 190, &regs->hfscr);
	printf("HFSCR : 0x%016" PRIx64 "\n", regs->hfscr);

	ram_getspr(thread, 306, &value);
	regs->hdsisr = value;
	printf("HDSISR: 0x%08" PRIx32 "\n", regs->hdsisr);

	ram_getspr(thread, 307, &regs->hdar);
	printf("HDAR  : 0x%016" PRIx64 "\n", regs->hdar);

	ram_getspr(thread, 314, &regs->hsrr0);
	printf("HSRR0 : 0x%016" PRIx64 "\n", regs->hsrr0);

	ram_getspr(thread, 315, &regs->hsrr1);
	printf("HSRR1 : 0x%016" PRIx64 "\n", regs->hsrr1);

	ram_getspr(thread, 310, &regs->hdec);
	printf("HDEC  : 0x%016" PRIx64 "\n", regs->hdec);

	ram_getspr(thread, 304, &regs->hsprg0);
	printf("HSPRG0: 0x%016" PRIx64 "\n", regs->hsprg0);

	ram_getspr(thread, 305, &regs->hsprg1);
	printf("HSPRG1: 0x%016" PRIx64 "\n", regs->hsprg1);

	ram_getspr(thread, 153, &regs->fscr);
	printf("FSCR  : 0x%016" PRIx64 "\n", regs->fscr);

	ram_getspr(thread, 18, &value);
	regs->dsisr = value;
	printf("DSISR : 0x%08" PRIx32 "\n", regs->dsisr);

	ram_getspr(thread, 19, &regs->dar);
	printf("DAR   : 0x%016" PRIx64 "\n", regs->dar);

	ram_getspr(thread, 26, &regs->srr0);
	printf("SRR0  : 0x%016" PRIx64 "\n", regs->srr0);

	ram_getspr(thread, 27, &regs->srr1);
	printf("SRR1  : 0x%016" PRIx64 "\n", regs->srr1);

	ram_getspr(thread, 22, &regs->dec);
	printf("DEC   : 0x%016" PRIx64 "\n", regs->dec);

	ram_getspr(thread, 268, &regs->tb);
	printf("TB    : 0x%016" PRIx64 "\n", regs->tb);

	ram_getspr(thread, 272, &regs->sprg0);
	printf("SPRG0 : 0x%016" PRIx64 "\n", regs->sprg0);

	ram_getspr(thread, 273, &regs->sprg1);
	printf("SPRG1 : 0x%016" PRIx64 "\n", regs->sprg1);

	ram_getspr(thread, 274, &regs->sprg2);
	printf("SPRG2 : 0x%016" PRIx64 "\n", regs->sprg2);

	ram_getspr(thread, 275, &regs->sprg3);
	printf("SPRG3 : 0x%016" PRIx64 "\n", regs->sprg3);

	ram_getspr(thread, 896, &regs->ppr);
	printf("PPR   : 0x%016" PRIx64 "\n", regs->ppr);

	CHECK_ERR(t->ram_destroy(t));

	return 0;
}
