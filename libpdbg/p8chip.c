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
#define HID0_REG                 	0x1329c
#define EN_ATTN                 	PPC_BIT(31)

/* p8 specific opcodes for instruction ramming*/
#define MTXERF0_OPCODE 0x00000008UL
#define MTXERF1_OPCODE 0x00000108UL
#define MTXERF2_OPCODE 0x00000208UL
#define MTXERF3_OPCODE 0x00000308UL
#define MFXERF0_OPCODE 0x00000010UL
#define MFXERF1_OPCODE 0x00000110UL
#define MFXERF2_OPCODE 0x00000210UL
#define MFXERF3_OPCODE 0x00000310UL

/* How long (in us) to wait for a special wakeup to complete */
#define SPECIAL_WKUP_TIMEOUT		10

#include "chip.h"

static uint64_t mfxerf(uint64_t reg, uint64_t field)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mfxerf\n");

	switch (field) {
	case 0:
		return MFXERF0_OPCODE | (reg << 21);
	case 1:
		return MFXERF1_OPCODE | (reg << 21);
	case 2:
		return MFXERF2_OPCODE | (reg << 21);
	case 3:
		return MFXERF3_OPCODE | (reg << 21);
	default:
		PR_ERROR("Invalid XER field specified\n");
	}
	return 0;
}

static uint64_t mtxerf(uint64_t reg, uint64_t field)
{
	if (reg > 31)
		PR_ERROR("Invalid register specified for mtxerf\n");

	switch (field) {
	case 0:
		return MTXERF0_OPCODE | (reg << 21);
	case 1:
		return MTXERF1_OPCODE | (reg << 21);
	case 2:
		return MTXERF2_OPCODE | (reg << 21);
	case 3:
		return MTXERF3_OPCODE | (reg << 21);
	default:
		PR_ERROR("Invalid XER field specified\n");
	}
	return 0;
}

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
	pib_write(&thread->target, RAS_MODE_REG, mode_reg | MR_THREAD_IN_DEBUG);

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
	struct core *chip = target_to_core(
		pdbg_target_require_parent("core", &thread->target));

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
	struct core *chip = target_to_core(
		pdbg_target_require_parent("core", &thread->target));

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
	struct core *chip = target_to_core(
		pdbg_target_require_parent("core", &thread->target));
	uint64_t ram_mode, val;

	if (thread->ram_is_setup)
		return 1;

	/* We can only ram a thread if all the threads on the core/chip are
	 * quiesced */
	dt_for_each_compatible(&chip->target, target, "ibm,power8-thread") {
		struct thread *tmp;

		/* If this thread wasn't enabled it may not yet have been probed
		   so do that now. This will also update the thread status */
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			return 1;

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
	struct core *chip = target_to_core(
		pdbg_target_require_parent("core", &thread->target));
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
	struct core *chip = target_to_core(
		pdbg_target_require_parent("core", &thread->target));
	uint64_t ram_mode;

	/* Disable RAM mode */
	CHECK_ERR(pib_read(&chip->target, RAM_MODE_REG, &ram_mode));
	ram_mode &= ~RAM_MODE_ENABLE;
	CHECK_ERR(pib_write(&chip->target, RAM_MODE_REG, ram_mode));

	thread->ram_is_setup = false;

	return 0;
}

static int p8_ram_getxer(struct pdbg_target *thread, uint64_t *value)
{
	uint64_t opcodes[] = {mfxerf(0, 0), mtspr(277, 0), mfxerf(0, 1),
			      mtspr(277, 0), mfxerf(0, 2), mtspr(277, 0),
			      mfxerf(0, 3), mtspr(277, 0)};
	uint64_t results[] = {0, 0, 0, 0, 0, 0, 0, 0};

	/* On POWER8 we can't get xer with getspr. We seem to only be able to
	 * get and set IBM bits 32-34 and 44-56.
	 */
	PR_WARNING("Can only get/set IBM bits 32-34 and 44-56 of the XER register\n");


	CHECK_ERR(ram_instructions(thread, opcodes, results, ARRAY_SIZE(opcodes), 0));

	*value = results[1] | results[3] | results[5] | results[7];
	return 0;
}

static int p8_ram_putxer(struct pdbg_target *thread, uint64_t value)
{
	uint64_t fields[] = {value, value, value, value, 0};
	uint64_t opcodes[] = {mfspr(0, 277), mtxerf(0, 0), mtxerf(0, 1), mtxerf(0, 2), mtxerf(0, 3)};

	/* We seem to only be able to get and set IBM bits 32-34 and 44-56.*/
	PR_WARNING("Can only set IBM bits 32-34 and 44-56 of the XER register\n");

	CHECK_ERR(ram_instructions(thread, opcodes, fields, ARRAY_SIZE(opcodes), 0));

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

static int p8_get_hid0(struct pdbg_target *chip, uint64_t *value)
{
	CHECK_ERR(pib_read(chip, HID0_REG, value));
	return 0;
}

static int p8_put_hid0(struct pdbg_target *chip, uint64_t value)
{
	CHECK_ERR(pib_write(chip, HID0_REG, value));
	return 0;
}

static int p8_enable_attn(struct pdbg_target *target)
{
	struct pdbg_target *core;
	uint64_t hid0;

	core = pdbg_target_parent("core", target);
	if (core == NULL)
	{
		PR_ERROR("CORE NOT FOUND\n");
		return 1;
	}

	/* Need to enable the attn instruction in HID0 */
	if (p8_get_hid0(core, &hid0)) {
		PR_ERROR("Unable to get HID0\n");
		return 1;
	}
	hid0 |= EN_ATTN;

	if (p8_put_hid0(core, hid0)) {
		PR_ERROR("Unable to set HID0\n");
		return 1;
	}
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
	.ram_getxer = p8_ram_getxer,
	.ram_putxer = p8_ram_putxer,
	.enable_attn = p8_enable_attn,
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
