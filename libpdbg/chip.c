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

#include "hwunit.h"
#include "operations.h"
#include "bitutils.h"
#include "debug.h"

uint64_t mfspr(uint64_t reg, uint64_t spr)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mfspr\n");

	return MFSPR_OPCODE | (reg << 21) | ((spr & 0x1f) << 16) | ((spr & 0x3e0) << 6);
}

uint64_t mtspr(uint64_t spr, uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mtspr\n");

	return MTSPR_OPCODE | (reg << 21) | ((spr & 0x1f) << 16) | ((spr & 0x3e0) << 6);
}

static uint64_t mfocrf(uint64_t reg, uint64_t cr)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mfocrf\n");
	if (cr > 7)
		PR_ERROR("Invalid CR field specified\n");

	return MFOCRF_OPCODE | (reg << 21) | (1U << (12 + cr));
}

static uint64_t mtocrf(uint64_t cr, uint64_t reg)
{
	if (reg > 31) {
		PR_ERROR("Invalid register specified for mfocrf\n");
		exit(1);
	}
	if (cr > 7) {
		PR_ERROR("Invalid CR field specified\n");
		exit(1);
	}

	return MTOCRF_OPCODE | (reg << 21) | (1U << (12 + cr));
}

static uint64_t mfnia(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mfnia\n");

	return MFNIA_OPCODE | (reg << 21);
}

static uint64_t mtnia(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mtnia\n");

	return MTNIA_OPCODE | (reg << 21);
}

static uint64_t mfmsr(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mfmsr\n");

	return MFMSR_OPCODE | (reg << 21);
}

static uint64_t mtmsr(uint64_t reg)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mtmsr\n");

	return MTMSR_OPCODE | (reg << 21);
}

static uint64_t ld(uint64_t rt, uint64_t ds, uint64_t ra)
{
	if ((rt > 31) | (ra > 31) | (ds > 0x3fff))
		PR_ERROR("Invalid register specified\n");

	return LD_OPCODE | (rt << 21) | (ra << 16) | (ds << 2);
}

struct thread_state thread_status(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->status;
}

/*
 * Single step the thread count instructions.
 */
int thread_step(struct pdbg_target *target, int count)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->step(thread, count);
}

int thread_start(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->start(thread);
}

int thread_stop(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->stop(thread);
}

int thread_sreset(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->sreset(thread);
}

int thread_step_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (!pib->thread_step_all)
			break;

		rc |= pib->thread_step_all(pib, 1);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_step(thread, 1);
	}

	return rc;
}

int thread_start_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (!pib->thread_start_all)
			break;

		rc |= pib->thread_start_all(pib);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_start(thread);
	}

	return rc;
}

int thread_stop_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (!pib->thread_stop_all)
			break;

		rc |= pib->thread_stop_all(pib);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_stop(thread);
	}

	return rc;
}

int thread_sreset_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (!pib->thread_sreset_all)
			break;

		rc |= pib->thread_sreset_all(pib);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_sreset(thread);
	}

	return rc;
}

/*
 * RAMs the opcodes in *opcodes and store the results of each opcode
 * into *results. *results must point to an array the same size as
 * *opcodes. Each entry from *results is put into SCR0 prior to
 * executing an opcode so that it may also be used to pass in
 * data. Note that only registers r0 and r1 are saved and restored so
 * opcode sequences must preserve other registers.
 */
int ram_instructions(struct thread *thread, uint64_t *opcodes,
			    uint64_t *results, int len, unsigned int lpar)
{
	uint64_t opcode = 0, r0 = 0, r1 = 0, scratch = 0;
	int i;
	int exception = 0;
	bool did_setup = false;

	if (!thread->ram_is_setup) {
		CHECK_ERR(thread->ram_setup(thread));
		did_setup = true;
	}

	/* RAM instructions */
	for (i = -2; i < len + 2; i++) {
		if (i == -2)
			/* Save r1 (assumes opcodes don't touch other registers) */
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
			PR_DEBUG("%s: %d, %016" PRIx64 "\n", __FUNCTION__, __LINE__, opcode);
			exception = 1;
			if (i >= 0 && i < len)
				/* skip the rest and attempt to restore r0 and r1 */
				i = len - 1;
			else
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
int thread_getgpr(struct pdbg_target *target, int gpr, uint64_t *value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mtspr(277, gpr)};
	uint64_t results[] = {0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[0];
	return 0;
}

int thread_putgpr(struct pdbg_target *target, int gpr, uint64_t value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfspr(gpr, 277)};
	uint64_t results[] = {value};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));

	return 0;
}

int thread_getnia(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfnia(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

/*
 * P9 must MTNIA from LR, P8 can MTNIA from R0. So we set both LR and R0
 * to value. LR must be saved and restored.
 *
 * This is a hack and should be made much cleaner once we have target
 * specific putspr commands.
 */
int thread_putnia(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;
	uint64_t opcodes[] = {	mfspr(1, 8),	/* mflr r1 */
				mfspr(0, 277),	/* value -> r0 */
				mtspr(8, 0),	/* mtlr r0 */
				mtnia(0),
				mtspr(8, 1), };	/* mtlr r1 */
	uint64_t results[] = {0, value, 0, 0, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int thread_getspr(struct pdbg_target *target, int spr, uint64_t *value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfspr(0, spr), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int thread_putspr(struct pdbg_target *target, int spr, uint64_t value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfspr(0, 277), mtspr(spr, 0)};
	uint64_t results[] = {value, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int thread_getmsr(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfmsr(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int thread_getcr(struct pdbg_target *target, uint32_t *value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfocrf(0, 0), mtspr(277, 0), mfocrf(0, 1), mtspr(277, 0),
			      mfocrf(0, 2), mtspr(277, 0), mfocrf(0, 3), mtspr(277, 0),
			      mfocrf(0, 4), mtspr(277, 0), mfocrf(0, 5), mtspr(277, 0),
			      mfocrf(0, 6), mtspr(277, 0), mfocrf(0, 7), mtspr(277, 0)};
	uint64_t results[16] = {0};
	uint32_t cr_field, cr = 0;
	int i;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	for (i = 1; i < 16; i += 2) {
		cr_field = results[i];
		/* We are not guaranteed that the other bits will be zeroed out */
		cr |= cr_field & (0xf << 2*(i-1));
	}

	*value = cr;
	return 0;
}

int thread_putcr(struct pdbg_target *target, uint32_t value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfspr(0, 277), mtocrf(0, 0), mtocrf(1, 0),
			      mtocrf(2, 0), mtocrf(3, 0), mtocrf(4, 0),
			      mtocrf(5, 0), mtocrf(6, 0), mtocrf(7, 0)};
	uint64_t results[] = {value};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));

	return 0;
}

int thread_putmsr(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfspr(0, 277), mtmsr(0)};
	uint64_t results[] = {value, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int thread_getmem(struct pdbg_target *target, uint64_t addr, uint64_t *value)
{
	struct thread *thread;
	uint64_t opcodes[] = {mfspr(0, 277), mfspr(1, 277), ld(0, 0, 1), mtspr(277, 0)};
	uint64_t results[] = {0xdeaddeaddeaddead, addr, 0, 0};

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[3];
	return 0;
}

int thread_getxer(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	CHECK_ERR(thread->ram_getxer(thread, value));

	return 0;
}

int thread_putxer(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	CHECK_ERR(thread->ram_putxer(thread, value));

	return 0;
}

/*
 * Read the given ring from the given chiplet. Result must be large enough to hold ring_len bits.
 */
int getring(struct pdbg_target *target, uint64_t ring_addr, uint64_t ring_len, uint32_t result[])
{
	struct chiplet *chiplet;

	assert(pdbg_target_is_class(target, "chiplet"));
	chiplet = target_to_chiplet(target);
	return chiplet->getring(chiplet, ring_addr, ring_len, result);
}

int thread_getregs(struct pdbg_target *target, struct thread_regs *regs)
{
	struct thread *thread;
	struct thread_regs _regs;
	uint64_t value = 0;
	int i;

	if (!regs)
		regs = &_regs;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	CHECK_ERR(thread->ram_setup(thread));

	/*
	 * It would be neat to do all the ramming up front, then go through
	 * and print everything out somewhere else. In practice so far it
	 * can help to diagnose checkstop issues with ramming to print as
	 * we go. Once it's more robust and tested, maybe.
	 */
	thread_getnia(target, &regs->nia);
	printf("NIA   : 0x%016" PRIx64 "\n", regs->nia);

	thread_getspr(target, 28, &regs->cfar);
	printf("CFAR  : 0x%016" PRIx64 "\n", regs->cfar);

	thread_getmsr(target, &regs->msr);
	printf("MSR   : 0x%016" PRIx64 "\n", regs->msr);

	thread_getspr(target, 8, &regs->lr);
	printf("LR    : 0x%016" PRIx64 "\n", regs->lr);

	thread_getspr(target, 9, &regs->ctr);
	printf("CTR   : 0x%016" PRIx64 "\n", regs->ctr);

	thread_getspr(target, 815, &regs->tar);
	printf("TAR   : 0x%016" PRIx64 "\n", regs->tar);

	thread_getcr(target, &regs->cr);
	printf("CR    : 0x%08" PRIx32 "\n", regs->cr);

	thread_getxer(target, &regs->xer);
	printf("XER   : 0x%08" PRIx64 "\n", regs->xer);

	printf("GPRS  :\n");
	for (i = 0; i < 32; i++) {
		thread_getgpr(target, i, &regs->gprs[i]);
		printf(" 0x%016" PRIx64 "", regs->gprs[i]);
		if (i % 4 == 3)
			printf("\n");
	}

	thread_getspr(target, 318, &regs->lpcr);
	printf("LPCR  : 0x%016" PRIx64 "\n", regs->lpcr);

	thread_getspr(target, 464, &regs->ptcr);
	printf("PTCR  : 0x%016" PRIx64 "\n", regs->ptcr);

	thread_getspr(target, 319, &regs->lpidr);
	printf("LPIDR : 0x%016" PRIx64 "\n", regs->lpidr);

	thread_getspr(target, 48, &regs->pidr);
	printf("PIDR  : 0x%016" PRIx64 "\n", regs->pidr);

	thread_getspr(target, 190, &regs->hfscr);
	printf("HFSCR : 0x%016" PRIx64 "\n", regs->hfscr);

	thread_getspr(target, 306, &value);
	regs->hdsisr = value;
	printf("HDSISR: 0x%08" PRIx32 "\n", regs->hdsisr);

	thread_getspr(target, 307, &regs->hdar);
	printf("HDAR  : 0x%016" PRIx64 "\n", regs->hdar);

	thread_getspr(target, 339, &value);
	regs->heir = value;
	printf("HEIR : 0x%016" PRIx32 "\n", regs->heir);

	thread_getspr(target, 1008, &regs->hid);
	printf("HID0 : 0x%016" PRIx64 "\n", regs->hid);

	thread_getspr(target, 314, &regs->hsrr0);
	printf("HSRR0 : 0x%016" PRIx64 "\n", regs->hsrr0);

	thread_getspr(target, 315, &regs->hsrr1);
	printf("HSRR1 : 0x%016" PRIx64 "\n", regs->hsrr1);

	thread_getspr(target, 310, &regs->hdec);
	printf("HDEC  : 0x%016" PRIx64 "\n", regs->hdec);

	thread_getspr(target, 304, &regs->hsprg0);
	printf("HSPRG0: 0x%016" PRIx64 "\n", regs->hsprg0);

	thread_getspr(target, 305, &regs->hsprg1);
	printf("HSPRG1: 0x%016" PRIx64 "\n", regs->hsprg1);

	thread_getspr(target, 153, &regs->fscr);
	printf("FSCR  : 0x%016" PRIx64 "\n", regs->fscr);

	thread_getspr(target, 18, &value);
	regs->dsisr = value;
	printf("DSISR : 0x%08" PRIx32 "\n", regs->dsisr);

	thread_getspr(target, 19, &regs->dar);
	printf("DAR   : 0x%016" PRIx64 "\n", regs->dar);

	thread_getspr(target, 26, &regs->srr0);
	printf("SRR0  : 0x%016" PRIx64 "\n", regs->srr0);

	thread_getspr(target, 27, &regs->srr1);
	printf("SRR1  : 0x%016" PRIx64 "\n", regs->srr1);

	thread_getspr(target, 22, &regs->dec);
	printf("DEC   : 0x%016" PRIx64 "\n", regs->dec);

	thread_getspr(target, 268, &regs->tb);
	printf("TB    : 0x%016" PRIx64 "\n", regs->tb);

	thread_getspr(target, 272, &regs->sprg0);
	printf("SPRG0 : 0x%016" PRIx64 "\n", regs->sprg0);

	thread_getspr(target, 273, &regs->sprg1);
	printf("SPRG1 : 0x%016" PRIx64 "\n", regs->sprg1);

	thread_getspr(target, 274, &regs->sprg2);
	printf("SPRG2 : 0x%016" PRIx64 "\n", regs->sprg2);

	thread_getspr(target, 275, &regs->sprg3);
	printf("SPRG3 : 0x%016" PRIx64 "\n", regs->sprg3);

	thread_getspr(target, 896, &regs->ppr);
	printf("PPR   : 0x%016" PRIx64 "\n", regs->ppr);

	CHECK_ERR(thread->ram_destroy(thread));

	return 0;
}

static struct proc proc = {
	.target = {
		.name = "Processor Module",
		.compatible = "ibm,power-proc",
		.class = "proc",
	},
};
DECLARE_HW_UNIT(proc);

__attribute__((constructor))
static void register_proc(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &proc_hw_unit);
}
