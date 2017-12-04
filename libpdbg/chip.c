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
#include <string.h>
#include <stdlib.h>
#include <ccan/array_size/array_size.h>
#include <unistd.h>

#include "target.h"
#include "operations.h"
#include "bitutils.h"

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

	assert(!strcmp(thread_target->class, "thread"));
	thread = target_to_thread(thread_target);
	CHECK_ERR(thread->ram_setup(thread));

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

		CHECK_ERR(thread->ram_instruction(thread, opcode, &scratch));

		if (i == -2)
			r1 = scratch;
		else if (i == -1)
			r0 = scratch;
		else if (i < len)
			results[i] = scratch;
	}

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
