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

#include "operations.h"
#include "bitutils.h"

#define RAS_STATUS_TIMEOUT	100

#define DIRECT_CONTROLS_REG    		0x10013000
#define  DIRECT_CONTROL_SP_START 	PPC_BIT(62)
#define  DIRECT_CONTROL_SP_STOP 	PPC_BIT(63)
#define RAS_STATUS_REG			0x10013002
#define  RAS_STATUS_SRQ_EMPTY		PPC_BIT(8)
#define  RAS_STATUS_NCU_QUIESCED 	PPC_BIT(10)
#define  RAS_STATUS_TS_QUIESCE		PPC_BIT(49)
#define THREAD_ACTIVE_REG      		0x1001310E
#define  THREAD_ACTIVE			PPC_BITMASK(0, 7)
#define  RAM_THREAD_ACTIVE		PPC_BITMASK(8, 15)
#define SPR_MODE_REG			0x10013281
#define  SPR_MODE_SPRC_WR_EN		PPC_BIT(3)
#define  SPR_MODE_SPRC_SEL		PPC_BITMASK(16, 19)
#define  SPR_MODE_SPRC_T_SEL   		PPC_BITMASK(20, 27)
#define L0_SCOM_SPRC_REG		0x10013280
#define  SCOM_SPRC_SCRATCH_SPR		0x40
#define SCR0_REG 			0x10013283
#define RAM_MODE_REG			0x10013c00
#define  RAM_MODE_ENABLE		PPC_BIT(0)
#define RAM_CTRL_REG			0x10013c01
#define  RAM_THREAD_SELECT		PPC_BITMASK(0, 2)
#define  RAM_INSTR			PPC_BITMASK(3, 34)
#define RAM_STATUS_REG			0x10013c02
#define  RAM_CONTROL_RECOV		PPC_BIT(0)
#define  RAM_STATUS			PPC_BIT(1)
#define  RAM_EXCEPTION			PPC_BIT(2)
#define  LSU_EMPTY			PPC_BIT(3)
#define PMSPCWKUPFSP_REG		0x100f010b
#define  FSP_SPECIAL_WAKEUP		PPC_BIT(0)

/* Opcodes */
#define MFNIA_OPCODE 0x00000004UL
#define MFSPR_OPCODE 0x7c0002a6UL
#define MTSPR_OPCODE 0x7c0003a6UL

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

/*
 * Return a mask of currently active threads.
 */
int ram_running_threads(uint64_t chip, uint64_t *running_threads)
{
	uint64_t addr, val;
	int thread;

	chip <<= 24;
	*running_threads = 0;

	for (thread = 0; thread < THREADS_PER_CORE; thread++) {
		/* Wait for thread to quiese */
		addr = RAS_STATUS_REG | chip;
		addr |= thread << 4;
		CHECK_ERR(getscom(&val, addr));

		if (!((val & RAS_STATUS_SRQ_EMPTY)
		      && (val & RAS_STATUS_TS_QUIESCE)))
			*running_threads |= 1 << (THREADS_PER_CORE - thread - 1);
	}
	return 0;
}

/*
 * Stop active threads on the chip. Returns the active_threads in
 * *active_threads. This should be passed to ram_start_chip() to
 * restart the stopped threads.
 */
int ram_stop_chip(uint64_t chip, uint64_t *active_threads)
{
	int i = 0, thread;
	uint64_t addr, val, thread_active;

	chip <<= 24;

	/* Assert special wakeup to prevent low power states */
	addr = PMSPCWKUPFSP_REG	| chip;
	CHECK_ERR(putscom(FSP_SPECIAL_WAKEUP, addr));

	/* Read active threads */
	addr = THREAD_ACTIVE_REG | chip;
	CHECK_ERR(getscom(&val, addr));
	thread_active = GETFIELD(THREAD_ACTIVE, val);

	for (thread = 0; thread < THREADS_PER_CORE; thread++) {
		/* Skip inactive threads (stupid IBM bit ordering) */
		if (!(thread_active & (1 << (THREADS_PER_CORE - thread - 1))))
			continue;

		/* Quiese active thread */
		addr = DIRECT_CONTROLS_REG | chip;
		addr |= thread << 4;
		val = DIRECT_CONTROL_SP_STOP;
		CHECK_ERR(putscom(val, addr));

		/* Wait for thread to quiese */
		addr = RAS_STATUS_REG | chip;
		addr |= thread << 4;
		i = 0;
		do {
			CHECK_ERR(getscom(&val, addr));
			if (i++ > RAS_STATUS_TIMEOUT) {
				PR_ERROR("Unable to quiesce thread %d (0x%016llx).\n",
					 thread, val);
				PR_ERROR("Continuing anyway.\n");
				break;
			}
		} while (!((val & RAS_STATUS_SRQ_EMPTY)
			   /*
			    * Workbook says check for bit 10 but it
			    * never seems to get set and everything
			    * still works as expected.
			    */
                           /* && (val & RAS_STATUS_NCU_QUIESCED) */
			   && (val & RAS_STATUS_TS_QUIESCE)));
	}

	/* Make the RAM threads active */
	addr = THREAD_ACTIVE_REG | chip;
	CHECK_ERR(getscom(&val, addr));
	val = SETFIELD(RAM_THREAD_ACTIVE, val, (1 << THREADS_PER_CORE) - 1);
	CHECK_ERR(putscom(val, addr));

	/* Activate RAM mode */
	addr = RAM_MODE_REG | chip;
	CHECK_ERR(getscom(&val, addr));
	val |= RAM_MODE_ENABLE;
	CHECK_ERR(putscom(val, addr));

	*active_threads = thread_active;
	return 0;
}

int ram_start_chip(uint64_t chip, uint64_t thread_active)
{
	uint64_t addr, val;
	int thread;

	chip <<= 24;

	/* De-activate RAM mode */
	addr = RAM_MODE_REG | chip;
	CHECK_ERR(putscom(0, addr));

	/* Start cores which were active */
	for (thread = 0; thread < THREADS_PER_CORE; thread++) {
		if (!(thread_active & (1 << (THREADS_PER_CORE - thread - 1))))
			continue;

		/* Activate thread */
		addr = DIRECT_CONTROLS_REG | chip;
		addr |= thread << 4;
		val = DIRECT_CONTROL_SP_START;
		CHECK_ERR(putscom(val, addr));
	}

	/* De-assert special wakeup to enable low power states */
	addr = PMSPCWKUPFSP_REG	| chip;
	CHECK_ERR(putscom(0, addr));

	return 0;
}

/*
 * RAMs the opcodes in *opcodes and store the results of each opcode
 * into *results. *results must point to an array the same size as
 * *opcodes. Note that only register r0 is saved and restored so
 * opcodes must not touch other registers.
 */
static int ram_instructions(uint64_t *opcodes, uint64_t *results, int len,
			    uint64_t chip, unsigned int lpar, uint64_t thread)
{
	uint64_t addr, val, opcode, r0 = 0;
	int i;

	/* Setup SPRC to use SPRD */
	chip <<= 24;
	addr = SPR_MODE_REG | chip;
	val = SPR_MODE_SPRC_WR_EN;
	val = SETFIELD(SPR_MODE_SPRC_SEL, val, 1 << (3 - lpar));
	val = SETFIELD(SPR_MODE_SPRC_T_SEL, val, 1 << (7 - thread));
	CHECK_ERR(putscom(val, addr));

	addr = L0_SCOM_SPRC_REG | chip;
	val = SCOM_SPRC_SCRATCH_SPR;
	CHECK_ERR(putscom(val, addr));

	/* RAM instructions */
	addr = RAM_CTRL_REG | chip;
	for (i = -1; i <= len; i++) {
		if (i < 0)
			/* Save r0 (assumes opcodes don't touch other registers) */
			opcode = mtspr(277, 0);
		else if (i < len)
			opcode = opcodes[i];
		else if (i >= len) {
			/* Restore r0 */
			CHECK_ERR(putscom(r0, SCR0_REG | chip));
			opcode = mfspr(0, 277);
		}

		/* ram instruction */
		val = SETFIELD(RAM_THREAD_SELECT, 0ULL, thread);
		val = SETFIELD(RAM_INSTR, val, opcode);
		CHECK_ERR(putscom(val, addr));

		/* wait for completion */
		do {
			CHECK_ERR(getscom(&val, RAM_STATUS_REG | chip));
		} while (!val);

		if (!(val & RAM_STATUS))
			PR_ERROR("RAMMING failed with status 0x%llx\n", val);

		/* Save the results */
		CHECK_ERR(getscom(&val, SCR0_REG | chip));
		if (i < 0)
			r0 = val;
		else if (i < len)
			results[i] = val;
	}

	return 0;
}

/*
 * Get gpr value. Chip must be stopped.
 */
int ram_getgpr(int chip, int thread, int gpr, uint64_t *value)
{
	uint64_t opcodes[] = {mtspr(277, gpr)};
	uint64_t results[] = {0};

	CHECK_ERR(ram_instructions(opcodes, results, ARRAY_SIZE(opcodes),
				   chip, 0, thread));
	*value = results[0];
	return 0;
}

int ram_getnia(int chip, int thread, uint64_t *value)
{
	uint64_t opcodes[] = {mfnia(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(opcodes, results, ARRAY_SIZE(opcodes),
				   chip, 0, thread));
	*value = results[0];
	return 0;
}

int ram_getspr(int chip, int thread, int spr, uint64_t *value)
{
	uint64_t opcodes[] = {mfspr(0, spr), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(opcodes, results, ARRAY_SIZE(opcodes),
				   chip, 0, thread));
	*value = results[1];
	return 0;
}
