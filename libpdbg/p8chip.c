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
#include <inttypes.h>

#include "target.h"
#include "operations.h"
#include "bitutils.h"
#include "debug.h"

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
#define   PMC_POW_STATE_RUN		0x0
#define   PMC_POW_STATE_DOZE		0x1
#define   PMC_POW_STATE_NAP		0x2
#define   PMC_POW_STATE_SLEEP		0x3
#define  PMC_POW_SMT			PPC_BITMASK(6, 8)
#define   PMC_POW_SMT_0			0x0
#define   PMC_POW_SMT_1			0x1
#define   PMC_POW_SMT_2SH		0x2
#define   PMC_POW_SMT_2SP		0x3
#define   PMC_POW_SMT_4_3		0x4
#define   PMC_POW_SMT_4_4		0x6
#define   PMC_POW_SMT_8_5		0x5
#define   PMC_POW_SMT_8_8		0x7
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

static int assert_special_wakeup(struct core *chip)
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
			PR_ERROR("Timeout waiting for special wakeup on %s@0x%08" PRIx64 "\n", chip->target.name,
				 dt_get_address(&chip->target, 0, NULL));
			return -1;
		}
	} while (!(gp0 & SPECIAL_WKUP_DONE));

	return 0;
}

#if 0
/* TODO: Work out when to do this. */
static int deassert_special_wakeup(struct core *chip)
{
	/* Assert special wakeup to prevent low power states */
	CHECK_ERR(pib_write(&chip->target, PMSPCWKUPFSP_REG, 0));

	return 0;
}
#endif

static struct thread_state get_thread_status(struct thread *thread)
{
	uint64_t val, mode_reg;
	struct thread_state thread_status;

	/* Need to activete debug mode to get complete status */
	pib_read(&thread->target, RAS_MODE_REG, &mode_reg);
	mode_reg |= MR_THREAD_IN_DEBUG;
	pib_write(&thread->target, RAS_MODE_REG, mode_reg);

	/* Read status */
	pib_read(&thread->target, RAS_STATUS_REG, &val);

	thread_status.active = !!(val & RAS_STATUS_THREAD_ACTIVE);
	thread_status.quiesced = !!(val & RAS_STATUS_TS_QUIESCE);

	/* Read POW status */
	pib_read(&thread->target, POW_STATUS_REG, &val);

	switch (GETFIELD(PMC_POW_STATE, val)) {
	case PMC_POW_STATE_RUN:
		thread_status.sleep_state = PDBG_THREAD_STATE_RUN;
		break;

	case PMC_POW_STATE_DOZE:
		thread_status.sleep_state = PDBG_THREAD_STATE_DOZE;
		break;

	case PMC_POW_STATE_NAP:
		thread_status.sleep_state = PDBG_THREAD_STATE_NAP;
		break;

	case PMC_POW_STATE_SLEEP:
		thread_status.sleep_state = PDBG_THREAD_STATE_SLEEP;
		break;

	default:
		/* PMC_POW_STATE is a 2-bit field and we test all values so it
		 * should be impossible to get here. */
		assert(0);
	}

	switch (GETFIELD(PMC_POW_SMT, val)) {
	case PMC_POW_SMT_0:
		thread_status.smt_state = PDBG_SMT_UNKNOWN;
		break;

	case PMC_POW_SMT_1:
		thread_status.smt_state = PDBG_SMT_1;
		break;

	case PMC_POW_SMT_2SH:
	case PMC_POW_SMT_2SP:
		thread_status.smt_state = PDBG_SMT_2;
		break;

	/* It's unclear from the documentation what the difference between these
	 * two are. */
	case PMC_POW_SMT_4_3:
	case PMC_POW_SMT_4_4:
		thread_status.smt_state = PDBG_SMT_4;
		break;

	/* Ditto */
	case PMC_POW_SMT_8_5:
	case PMC_POW_SMT_8_8:
		thread_status.smt_state = PDBG_SMT_8;
		break;

	default:
		assert(0);
	}

	/* Clear debug mode */
	mode_reg &= ~MR_THREAD_IN_DEBUG;
	pib_write(&thread->target, RAS_MODE_REG, mode_reg);

	return thread_status;
}

static int p8_thread_step(struct thread *thread, int count)
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

static int p8_thread_stop(struct thread *thread)
{
	int i = 0;
	uint64_t val;
	struct core *chip = target_to_core(thread->target.parent);

	do {
		/* Quiese active thread */
		CHECK_ERR(pib_write(&thread->target, DIRECT_CONTROLS_REG, DIRECT_CONTROL_SP_STOP));

		/* Wait for thread to quiese */
		CHECK_ERR(pib_read(&chip->target, RAS_STATUS_REG, &val));
		if (i++ > RAS_STATUS_TIMEOUT) {
			PR_ERROR("Unable to quiesce thread %d (0x%016" PRIx64 ").\n",
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

static int p8_thread_start(struct thread *thread)
{
	uint64_t val;
	struct core *chip = target_to_core(thread->target.parent);

	/* Activate thread */
	CHECK_ERR(pib_write(&thread->target, DIRECT_CONTROLS_REG, DIRECT_CONTROL_SP_START));

	/* Restore thread active */
	CHECK_ERR(pib_read(&chip->target, THREAD_ACTIVE_REG, &val));
	val &= ~(PPC_BIT(8) >> thread->id);
	val |= PPC_BIT(thread->id);
	CHECK_ERR(pib_write(&chip->target, THREAD_ACTIVE_REG, val));

	return 0;
}

static int p8_thread_sreset(struct thread *thread)
{
	/* Broken on p8 */
	return 1;
}

static int p8_ram_setup(struct thread *thread)
{
	struct pdbg_target *target;
	struct core *chip = target_to_core(thread->target.parent);
	uint64_t ram_mode, val;

	if (thread->ram_is_setup)
		return 1;

	/* We can only ram a thread if all the threads on the core/chip are
	 * quiesced */
	dt_for_each_compatible(&chip->target, target, "ibm,power8-thread") {
		struct thread *tmp;
		tmp = target_to_thread(target);
		if (!(get_thread_status(tmp).quiesced))
			return 1;
	}

	if (!(thread->status.active))
		return 2;

	/* Activate RAM mode */
	CHECK_ERR(pib_read(&chip->target, RAM_MODE_REG, &ram_mode));
	ram_mode |= RAM_MODE_ENABLE;
	CHECK_ERR(pib_write(&chip->target, RAM_MODE_REG, ram_mode));

	/* Setup SPRC to use SPRD */
	val = SPR_MODE_SPRC_WR_EN;
	val = SETFIELD(SPR_MODE_SPRC_SEL, val, 1 << (3 - 0));
	val = SETFIELD(SPR_MODE_SPRC_T_SEL, val, 1 << (7 - thread->id));
	CHECK_ERR(pib_write(&chip->target, SPR_MODE_REG, val));
	CHECK_ERR(pib_write(&chip->target, L0_SCOM_SPRC_REG, SCOM_SPRC_SCRATCH_SPR));

	thread->ram_is_setup = true;

	return 0;
}

static int p8_ram_instruction(struct thread *thread, uint64_t opcode, uint64_t *scratch)
{
	struct core *chip = target_to_core(thread->target.parent);
	uint64_t val;

	if (!thread->ram_is_setup)
		return 1;

	CHECK_ERR(pib_write(&chip->target, SCR0_REG, *scratch));

	/* ram instruction */
	val = SETFIELD(RAM_THREAD_SELECT, 0ULL, thread->id);
	val = SETFIELD(RAM_INSTR, val, opcode);
	CHECK_ERR(pib_write(&chip->target, RAM_CTRL_REG, val));

	/* wait for completion */
	do {
		CHECK_ERR(pib_read(&chip->target, RAM_STATUS_REG, &val));
	} while (!((val & PPC_BIT(1)) || ((val & PPC_BIT(2)) && (val & PPC_BIT(3)))));

	if (!(val & PPC_BIT(1))) {
		if (GETFIELD(PPC_BITMASK(2,3), val) == 0x3) {
			return 1;
		} else {
			PR_ERROR("RAMMING failed with status 0x%" PRIx64 "\n", val);
			return 2;
		}
	}

	/* Save the results */
	CHECK_ERR(pib_read(&chip->target, SCR0_REG, scratch));

	return 0;
}

static int p8_ram_destroy(struct thread *thread)
{
	struct core *chip = target_to_core(thread->target.parent);
	uint64_t ram_mode;

	/* Disable RAM mode */
	CHECK_ERR(pib_read(&chip->target, RAM_MODE_REG, &ram_mode));
	ram_mode &= ~RAM_MODE_ENABLE;
	CHECK_ERR(pib_write(&chip->target, RAM_MODE_REG, ram_mode));

	thread->ram_is_setup = false;

	return 0;
}

/*
 * Initialise all viable threads for ramming on the given core.
 */
static int p8_thread_probe(struct pdbg_target *target)
{
	struct thread *thread = target_to_thread(target);

	thread->id = (dt_get_address(target, 0, NULL) >> 4) & 0xf;
	thread->status = get_thread_status(thread);

	return 0;
}

static struct thread p8_thread = {
	.target = {
		.name = "POWER8 Thread",
		.compatible = "ibm,power8-thread",
		.class = "thread",
		.probe = p8_thread_probe,
	},
	.step = p8_thread_step,
	.start = p8_thread_start,
	.stop = p8_thread_stop,
	.sreset = p8_thread_sreset,
	.ram_setup = p8_ram_setup,
	.ram_instruction = p8_ram_instruction,
	.ram_destroy = p8_ram_destroy,
};
DECLARE_HW_UNIT(p8_thread);

static int p8_core_probe(struct pdbg_target *target)
{
	uint64_t value;
	struct core *core = target_to_core(target);

	/* Work out if this chip is actually present */
	if (pib_read(target, SCOM_EX_GP3, &value)) {
		PR_DEBUG("Error reading chip GP3 register\n");
		return -1;
	}

	if (!GETFIELD(PPC_BIT(0), value))
		return -1;

	assert_special_wakeup(core);
	return 0;
}

static struct core p8_core = {
	.target = {
		.name = "POWER8 Core",
		.compatible = "ibm,power8-core",
		.class = "core",
		.probe = p8_core_probe,
	},
};
DECLARE_HW_UNIT(p8_core);
