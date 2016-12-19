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

#define RAS_STATUS_TIMEOUT	100

#define DIRECT_CONTROLS_REG    		0x0
#define  DIRECT_CONTROL_SP_STEP		PPC_BIT(61)
#define  DIRECT_CONTROL_SP_START 	PPC_BIT(62)
#define  DIRECT_CONTROL_SP_STOP 	PPC_BIT(63)
#define RAS_MODE_REG			0x1
#define  MR_THREAD_IN_DEBUG		PPC_BIT(43)
#define  MR_DO_SINGLE_MODE		PPC_BIT(50)
#define RAS_STATUS_REG			0x2
#define  RAS_STATUS_SRQ_EMPTY		PPC_BIT(8)
#define  RAS_STATUS_LSU_QUIESCED 	PPC_BIT(9)
#define  RAS_STATUS_INST_COMPLETE	PPC_BIT(12)
#define  RAS_STATUS_THREAD_ACTIVE	PPC_BIT(48)
#define  RAS_STATUS_TS_QUIESCE		PPC_BIT(49)
#define POW_STATUS_REG			0x4
#define  PMC_POW_STATE			PPC_BITMASK(4, 5)
#define  CORE_POW_STATE			PPC_BITMASK(23, 25)
#define THREAD_ACTIVE_REG      		0x1310e
#define  THREAD_ACTIVE			PPC_BITMASK(0, 7)
#define  RAM_THREAD_ACTIVE		PPC_BITMASK(8, 15)
#define SPR_MODE_REG			0x13281
#define  SPR_MODE_SPRC_WR_EN		PPC_BIT(3)
#define  SPR_MODE_SPRC_SEL		PPC_BITMASK(16, 19)
#define  SPR_MODE_SPRC_T_SEL   		PPC_BITMASK(20, 27)
#define L0_SCOM_SPRC_REG		0x13280
#define  SCOM_SPRC_SCRATCH_SPR		0x40
#define SCR0_REG 			0x13283
#define RAM_MODE_REG			0x13c00
#define  RAM_MODE_ENABLE		PPC_BIT(0)
#define RAM_CTRL_REG			0x13c01
#define  RAM_THREAD_SELECT		PPC_BITMASK(0, 2)
#define  RAM_INSTR			PPC_BITMASK(3, 34)
#define RAM_STATUS_REG			0x13c02
#define  RAM_CONTROL_RECOV		PPC_BIT(0)
#define  RAM_STATUS			PPC_BIT(1)
#define  RAM_EXCEPTION			PPC_BIT(2)
#define  LSU_EMPTY			PPC_BIT(3)
#define SCOM_EX_GP3			0xf0012
#define PMSPCWKUPFSP_REG		0xf010d
#define  FSP_SPECIAL_WAKEUP		PPC_BIT(0)
#define EX_PM_GP0_REG			0xf0100
#define  SPECIAL_WKUP_DONE		PPC_BIT(31)

/* How long (in us) to wait for a special wakeup to complete */
#define SPECIAL_WKUP_TIMEOUT		10

/* Opcodes */
#define MTNIA_OPCODE 0x00000002UL
#define MFNIA_OPCODE 0x00000004UL
#define MFMSR_OPCODE 0x7c0000a6UL
#define MTMSR_OPCODE 0x7c000124UL
#define MFSPR_OPCODE 0x7c0002a6UL
#define MTSPR_OPCODE 0x7c0003a6UL
#define LD_OPCODE 0xe8000000UL

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

static int assert_special_wakeup(struct chiplet *chip)
{
	int i = 0;
	uint64_t gp0;

	/* Assert special wakeup to prevent low power states */
	CHECK_ERR(pib_write(&chip->target, PMSPCWKUPFSP_REG, FSP_SPECIAL_WAKEUP));

	/* Poll for completion */
	do {
		usleep(1);
		CHECK_ERR(pib_read(&chip->target, EX_PM_GP0_REG, &gp0));

		if (i++ > SPECIAL_WKUP_TIMEOUT) {
			PR_ERROR("Timeout waiting for special wakeup on %s@0x%08lx\n", chip->target.name,
				 dt_get_address(chip->target.dn, 0, NULL));
			return -1;
		}
	} while (!(gp0 & SPECIAL_WKUP_DONE));

	return 0;
}

static int deassert_special_wakeup(struct chiplet *chip)
{
	/* Assert special wakeup to prevent low power states */
	CHECK_ERR(pib_write(&chip->target, PMSPCWKUPFSP_REG, 0));

	return 0;
}

uint64_t thread_status(struct thread *thread)
{
	return thread->status;
}

static uint64_t get_thread_status(struct thread *thread)
{
	uint64_t val, mode_reg, thread_status = thread->status;

	/* Need to activete debug mode to get complete status */
	CHECK_ERR(pib_read(&thread->target, RAS_MODE_REG, &mode_reg));
	mode_reg |= MR_THREAD_IN_DEBUG;
	CHECK_ERR(pib_write(&thread->target, RAS_MODE_REG, mode_reg));

	/* Read status */
	CHECK_ERR(pib_read(&thread->target, RAS_STATUS_REG, &val));

	thread_status = SETFIELD(THREAD_STATUS_ACTIVE, thread_status, !!(val & RAS_STATUS_THREAD_ACTIVE));
	thread_status = SETFIELD(THREAD_STATUS_QUIESCE, thread_status, !!(val & RAS_STATUS_TS_QUIESCE));

	/* Read POW status */
	CHECK_ERR(pib_read(&thread->target, POW_STATUS_REG, &val));
	thread_status = SETFIELD(THREAD_STATUS_STATE, thread_status, GETFIELD(PMC_POW_STATE, val));

	/* Clear debug mode */
	mode_reg &= ~MR_THREAD_IN_DEBUG;
	CHECK_ERR(pib_write(&thread->target, RAS_MODE_REG, mode_reg));

	return thread_status;
}

/*
 * Single step the thread count instructions.
 */
int ram_step_thread(struct thread *thread, int count)
{
	int i;
	uint64_t ras_mode, ras_status;

	/* Activate single-step mode */
	CHECK_ERR(pib_read(&thread->target, RAS_MODE_REG, &ras_mode));
	ras_mode |= MR_DO_SINGLE_MODE;
	CHECK_ERR(pib_write(&thread->target, RAS_MODE_REG, ras_mode));

	/* Step the core */
	for (i = 0; i < count; i++) {
		CHECK_ERR(pib_write(&thread->target, DIRECT_CONTROLS_REG, DIRECT_CONTROL_SP_STEP));

		/* Wait for step to complete */
		do {
			CHECK_ERR(pib_read(&thread->target, RAS_STATUS_REG, &ras_status));
		} while (!(ras_status & RAS_STATUS_INST_COMPLETE));
	}

	/* Deactivate single-step mode */
	ras_mode &= ~MR_DO_SINGLE_MODE;
	CHECK_ERR(pib_write(&thread->target, RAS_MODE_REG, ras_mode));

	return 0;
}

int ram_stop_thread(struct thread *thread)
{
	int i = 0;
	uint64_t val;
	struct chiplet *chip = target_to_chiplet(thread->target.dn->parent->target);

	do {
		/* Quiese active thread */
		CHECK_ERR(pib_write(&thread->target, DIRECT_CONTROLS_REG, DIRECT_CONTROL_SP_STOP));

		/* Wait for thread to quiese */
		CHECK_ERR(pib_read(&chip->target, RAS_STATUS_REG, &val));
		if (i++ > RAS_STATUS_TIMEOUT) {
			PR_ERROR("Unable to quiesce thread %d (0x%016llx).\n",
				 thread->id, val);
			PR_ERROR("Continuing anyway.\n");
			if (val & PPC_BIT(48)) {
				PR_ERROR("Unable to continue\n");
			}
			break;
		}

		/* We can continue ramming if either the
		 * thread is not active or the SRQ/LSU/TS bits
		 * are set. */
	} while ((val & RAS_STATUS_THREAD_ACTIVE) &&
		 !((val & RAS_STATUS_SRQ_EMPTY)
		   && (val & RAS_STATUS_LSU_QUIESCED)
		   && (val & RAS_STATUS_TS_QUIESCE)));


	/* Make the threads RAM thread active */
	CHECK_ERR(pib_read(&chip->target, THREAD_ACTIVE_REG, &val));
	val |= PPC_BIT(8) >> thread->id;
	CHECK_ERR(pib_write(&chip->target, THREAD_ACTIVE_REG, val));

	return 0;
}

int ram_start_thread(struct thread *thread)
{
	uint64_t val;
	struct chiplet *chip = target_to_chiplet(thread->target.dn->parent->target);

	/* Activate thread */
	CHECK_ERR(pib_write(&thread->target, DIRECT_CONTROLS_REG, DIRECT_CONTROL_SP_START));

	/* Restore thread active */
	CHECK_ERR(pib_read(&chip->target, THREAD_ACTIVE_REG, &val));
	val &= ~(PPC_BIT(8) >> thread->id);
	val |= PPC_BIT(thread->id);
	CHECK_ERR(pib_write(&chip->target, THREAD_ACTIVE_REG, val));

	return 0;
}

/* We can only ram a thread if all the threads on the core/chip are
 * quiesced */
int ram_status(struct chiplet *chip)
{
	struct dt_node *dn;

	dt_for_each_compatible(chip->target.dn, dn, "ibm,power8-thread") {
		struct thread *thread;
		thread = target_to_thread(dn->target);
		if (!(get_thread_status(thread) & THREAD_STATUS_QUIESCE))
			return -1;
	}

	return 0;
}

/*
 * RAMs the opcodes in *opcodes and store the results of each opcode
 * into *results. *results must point to an array the same size as
 * *opcodes. Each entry from *results is put into SCR0 prior to
 * executing an opcode so that it may also be used to pass in
 * data. Note that only register r0 is saved and restored so opcodes
 * must not touch other registers.
 */
static int ram_instructions(struct thread *thread, uint64_t *opcodes,
			    uint64_t *results, int len, unsigned int lpar)
{
	uint64_t ram_mode, val, opcode, r0 = 0, r1 = 0;
	struct chiplet *chip = target_to_chiplet(thread->target.dn->parent->target);
	int i;
	int exception = 0;

	/* Check to see if the parent chip is in a state that can RAM instructions */
	if (ram_status(chip))
		return 1;

	if (!(thread_status(thread) & THREAD_STATUS_ACTIVE))
		return 2;

	/* Activate RAM mode */
	CHECK_ERR(pib_read(&chip->target, RAM_MODE_REG, &ram_mode));
	ram_mode |= RAM_MODE_ENABLE;
	CHECK_ERR(pib_write(&chip->target, RAM_MODE_REG, ram_mode));

	/* Setup SPRC to use SPRD */
	val = SPR_MODE_SPRC_WR_EN;
	val = SETFIELD(SPR_MODE_SPRC_SEL, val, 1 << (3 - lpar));
	val = SETFIELD(SPR_MODE_SPRC_T_SEL, val, 1 << (7 - thread->id));
	CHECK_ERR(pib_write(&chip->target, SPR_MODE_REG, val));
	CHECK_ERR(pib_write(&chip->target, L0_SCOM_SPRC_REG, SCOM_SPRC_SCRATCH_SPR));

	/* RAM instructions */
	for (i = -2; i < len + 2; i++) {
		if (i == -2)
			opcode = mtspr(277, 1);
		else if (i == -1)
			/* Save r0 (assumes opcodes don't touch other registers) */
			opcode = mtspr(277, 0);
		else if (i < len) {
			CHECK_ERR(pib_write(&chip->target, SCR0_REG, results[i]));
			opcode = opcodes[i];
		} else if (i == len) {
			/* Restore r0 */
			CHECK_ERR(pib_write(&chip->target, SCR0_REG, r0));
			opcode = mfspr(0, 277);
		} else if (i == len + 1) {
			/* Restore r1 */
			CHECK_ERR(pib_write(&chip->target, SCR0_REG, r1));
			opcode = mfspr(0, 277);
		}

		/* ram instruction */
		val = SETFIELD(RAM_THREAD_SELECT, 0ULL, thread->id);
		val = SETFIELD(RAM_INSTR, val, opcode);
		CHECK_ERR(pib_write(&chip->target, RAM_CTRL_REG, val));

		/* wait for completion */
		do {
			CHECK_ERR(pib_read(&chip->target, RAM_STATUS_REG, &val));
		} while (!((val & PPC_BIT(1)) || ((val & PPC_BIT(2)) && (val & PPC_BIT(3)))));

		if (!(val & PPC_BIT(1))) {
			exception = GETFIELD(PPC_BITMASK(2,3), val) == 0x3;
			if (exception) {
				/* Skip remaining instructions */
				i = len - 1;
				continue;
			} else
				PR_ERROR("RAMMING failed with status 0x%llx\n", val);
		}

		/* Save the results */
		CHECK_ERR(pib_read(&chip->target, SCR0_REG, &val));
		if (i == -2)
			r1 = val;
		else if (i == -1)
			r0 = val;
		else if (i < len)
			results[i] = val;
	}

	/* Disable RAM mode */
	ram_mode &= ~RAM_MODE_ENABLE;
	CHECK_ERR(pib_write(&chip->target, RAM_MODE_REG, ram_mode));

	return exception;
}

/*
 * Get gpr value. Chip must be stopped.
 */
int ram_getgpr(struct thread *thread, int gpr, uint64_t *value)
{
	uint64_t opcodes[] = {mtspr(277, gpr)};
	uint64_t results[] = {0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[0];
	return 0;
}

int ram_putgpr(struct thread *thread, int gpr, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(gpr, 277)};
	uint64_t results[] = {value};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));

	return 0;
}

int ram_getnia(struct thread *thread, uint64_t *value)
{
	uint64_t opcodes[] = {mfnia(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_putnia(struct thread *thread, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mtnia(0)};
	uint64_t results[] = {value, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int ram_getspr(struct thread *thread, int spr, uint64_t *value)
{
	uint64_t opcodes[] = {mfspr(0, spr), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_putspr(struct thread *thread, int spr, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mtspr(spr, 0)};
	uint64_t results[] = {value, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int ram_getmsr(struct thread *thread, uint64_t *value)
{
	uint64_t opcodes[] = {mfmsr(0), mtspr(277, 0)};
	uint64_t results[] = {0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[1];
	return 0;
}

int ram_putmsr(struct thread *thread, uint64_t value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mtmsr(0)};
	uint64_t results[] = {value, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	return 0;
}

int ram_getmem(struct thread *thread, uint64_t addr, uint64_t *value)
{
	uint64_t opcodes[] = {mfspr(0, 277), mfspr(1, 277), ld(0, 0, 1), mtspr(277, 0)};
	uint64_t results[] = {0xdeaddeaddeaddead, addr, 0, 0};

	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));
	*value = results[3];
	return 0;
}

/*
 * Initialise all viable threads for ramming on the given chiplet.
 */
static int p8_thread_probe(struct target *target)
{
	struct thread *thread = target_to_thread(target);

	thread->id = (dt_get_address(target->dn, 0, NULL) >> 4) & 0xf;
	thread->status = get_thread_status(thread);

	return 0;
}

struct thread p8_thread = {
	.target = {
		.name = "POWER8 Thread",
		.compatible = "ibm,power8-thread",
		.class = "thread",
		.probe = p8_thread_probe,
	},
};
DECLARE_HW_UNIT(p8_thread);

static int p8_chiplet_probe(struct target *target)
{
	uint64_t value;
	struct chiplet *chiplet = target_to_chiplet(target);
	int i, count = 0, rc = 0;

	/* Work out if this chip is actually present */
	if (pib_read(target, SCOM_EX_GP3, &value)) {
		PR_DEBUG("Error reading chip GP3 register\n");
		return -1;
	}

	if (!GETFIELD(PPC_BIT(0), value))
		return -1;

	assert_special_wakeup(chiplet);
	return 0;
}

struct chiplet p8_chiplet = {
	.target = {
		.name = "POWER8 Chiplet",
		.compatible = "ibm,power8-core",
		.class = "chiplet",
		.probe = p8_chiplet_probe,
	},
};
DECLARE_HW_UNIT(p8_chiplet);
