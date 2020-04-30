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
#include <unistd.h>
#include <inttypes.h>

#include "hwunit.h"
#include "operations.h"
#include "bitutils.h"
#include "debug.h"

/*
 * NOTE!
 * All timeouts and scom procedures in general through the file should be kept
 * in synch with skiboot (e.g., core/direct-controls.c) as far as possible.
 * If you fix a bug here, fix it in skiboot, and vice versa.
 */

#define P9_RAS_STATUS 0x10a02
#define P9_CORE_THREAD_STATE 0x10ab3
#define P9_THREAD_INFO 0x10a9b
#define P9_DIRECT_CONTROL 0x10a9c
#define P9_RAS_MODEREG 0x10a9d
#define P9_RAM_MODEREG 0x10a4e
#define P9_RAM_CTRL 0x10a4f
#define P9_RAM_STATUS 0x10a50
#define P9_SCOMC 0x10a80
#define P9_SPR_MODE 0x10a84
#define P9_SCR0_REG 0x10a86

#define CHIPLET_CTRL0_WOR	0x10
#define CHIPLET_CTRL0_CLEAR	0x20
#define  CHIPLET_CTRL0_CTRL_CC_ABIST_MUXSEL_DC	PPC_BIT(0)
#define  CHIPLET_CTRL0_TC_UNIT_SYNCCLK_MUXSEL_DC PPC_BIT(1)
#define  CHIPLET_CTRL0_CTRL_CC_FLUSHMODE_INH_DC	PPC_BIT(2)

#define CHIPLET_CTRL1_WOR	0x11
#define  CHIPLET_CTRL1_TC_VITL_REGION_FENCE	PPC_BIT(3)

#define CHIPLET_STAT0		0x100
#define  CHIPLET_STAT0_CC_CTRL_OPCG_DONE_DC	PPC_BIT(8)

#define CHIPLET_SCAN_REGION_TYPE	0x30005
#define CHIPLET_CLK_REGION		0x30006
#define  CHIPLET_CLK_REGION_CLOCK_CMD		PPC_BITMASK(0, 1)
#define  CHIPLET_CLK_REGION_CLOCK_CMD_STOP	0x2
#define  CHIPLET_CLK_REGION_SLAVE_MODE		PPC_BIT(2)
#define  CHIPLET_CLK_REGION_MASTER_MODE		PPC_BIT(3)
#define  CHIPLET_CLK_REGION_REGIONS		PPC_BITMASK(4, 14)
#define  CHIPLET_CLK_REGION_SEL_THOLD		PPC_BITMASK(48, 50)

/* PCB Slave Registers */
#define PPM_GPMMR	0xf0100
#define PPM_SPWKUP_FSP	0xf010b
#define PPM_SSHFSP	0xf0111
#define  SPECIAL_WKUP_DONE PPC_BIT(1)

#define RAS_STATUS_TIMEOUT	100 /* 100ms */
#define SPECIAL_WKUP_TIMEOUT	100 /* 100ms */

static uint64_t thread_read(struct thread *thread, uint64_t addr, uint64_t *data)
{
	struct pdbg_target *chip = pdbg_target_require_parent("core", &thread->target);

	return pib_read(chip, addr, data);
}

static uint64_t thread_write(struct thread *thread, uint64_t addr, uint64_t data)
{
	struct pdbg_target *chip = pdbg_target_require_parent("core", &thread->target);

	return pib_write(chip, addr, data);
}

static struct thread_state p9_get_thread_status(struct thread *thread)
{
	uint64_t value;
	struct thread_state thread_state;

	thread_read(thread, P9_RAS_STATUS, &value);

	thread_state.quiesced = (GETFIELD(PPC_BITMASK(8*thread->id, 3 + 8*thread->id), value) == 0xf);

	thread_read(thread, P9_THREAD_INFO, &value);
	thread_state.active = !!(value & PPC_BIT(thread->id));

	thread_read(thread, P9_CORE_THREAD_STATE, &value);
	if (value & PPC_BIT(56 + thread->id))
		thread_state.sleep_state = PDBG_THREAD_STATE_STOP;
	else
		thread_state.sleep_state = PDBG_THREAD_STATE_RUN;

	thread_state.smt_state = PDBG_SMT_UNKNOWN;

	return thread_state;
}

static int p9_thread_probe(struct pdbg_target *target)
{
	struct thread *thread = target_to_thread(target);
	uint32_t tid;

	assert(!pdbg_target_u32_property(target, "tid", &tid));
	thread->id = tid;
	thread->status = p9_get_thread_status(thread);

	return 0;
}

static void p9_thread_release(struct pdbg_target *target)
{
	struct core *core = target_to_core(pdbg_target_require_parent("core", target));
	struct thread *thread = target_to_thread(target);

	if (thread->status.quiesced)
		/* This thread is still quiesced so don't release spwkup */
		core->release_spwkup = false;}

static int p9_thread_start(struct thread *thread)
{
	if (!(thread->status.quiesced))
		return 1;

	if ((!(thread->status.active)) ||
	    (thread->status.sleep_state == PDBG_THREAD_STATE_STOP)) {
		/* Inactive or active ad stopped: Clear Maint */
		thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(3 + 8*thread->id));
	} else {
		/* Active and not stopped: Start */
		thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(6 + 8*thread->id));
	}

	thread->status = p9_get_thread_status(thread);

	return 0;
}

static int p9_thread_stop(struct thread *thread)
{
	int i = 0;

	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(7 + 8*thread->id));
	while (!(p9_get_thread_status(thread).quiesced)) {
		usleep(1000);
		if (i++ > RAS_STATUS_TIMEOUT) {
			PR_ERROR("Unable to quiesce thread\n");
			break;
		}
	}
	thread->status = p9_get_thread_status(thread);

	return 0;
}

static int p9_thread_step(struct thread *thread, int count)
{
	uint64_t value;
	int i;

	/* Can only step if a thread is quiesced */
	if (!(thread->status.quiesced))
		return 1;

	/* Core must be active to step */
	if (!(thread->status.active))
		return 1;

	/* Stepping a stop instruction doesn't really work */
	if (thread->status.sleep_state == PDBG_THREAD_STATE_STOP)
		return 1;

	/* Fence interrupts. */
	thread_write(thread, P9_RAS_MODEREG, PPC_BIT(57));

	/* Step the core */
	for (i = 0; i < count; i++) {
		/* Step */
		thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(5 + 8*thread->id));

		/* Poll PPC complete */
		do {
			thread_read(thread, P9_RAS_STATUS, &value);
		} while (!(value & PPC_BIT(4 + 8*thread->id)));
	}

	/* Un-fence */
	thread_write(thread, P9_RAS_MODEREG, 0);

	return 0;
}

static int p9_thread_sreset(struct thread *thread)
{
	/* Can only sreset if a thread is quiesced */
	if (!(thread->status.quiesced))
		return 1;

	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(4 + 8*thread->id));

	thread->status = p9_get_thread_status(thread);

	return 0;
}

static int p9_ram_setup(struct thread *thread)
{
	struct pdbg_target *target;
	struct core *chip = target_to_core(
		pdbg_target_require_parent("core", &thread->target));
	uint64_t value;

	if (thread->ram_is_setup)
		return 1;

	/* We can only ram a thread if all the threads on the core/chip are
	 * quiesced */
	pdbg_for_each_target("thread", &chip->target, target) {
		struct thread *tmp;

		/* If this thread wasn't enabled it may not yet have been probed
		   so do that now. This will also update the thread status */
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			goto out_fail;

		tmp = target_to_thread(target);
		if (!(tmp->status.quiesced))
			goto out_fail;
	}

	/* Wait for NEST_ACTIVE to clear */
	do {
		thread_read(thread, P9_RAS_STATUS, &value);
	} while (value & PPC_BIT(32));

	/* Wait for THREAD_ACTION_IN_PROGRESS to clear */
	do {
		thread_read(thread, P9_THREAD_INFO, &value);
	} while (value & PPC_BIT(23));

	/* Activate thread for ramming */
	CHECK_ERR_GOTO(out_fail,
		thread_write(thread, P9_THREAD_INFO, PPC_BIT(18 + thread->id)));

 	/* Enable ram mode */
	CHECK_ERR_GOTO(out_fail,
		thread_write(thread, P9_RAM_MODEREG, PPC_BIT(0)));

	/* Setup SPRC to use SPRD */
	CHECK_ERR_GOTO(out_fail,
		thread_write(thread, P9_SPR_MODE, 0x00000ff000000000));
	CHECK_ERR_GOTO(out_fail,
		thread_write(thread, P9_SCOMC, 0x0));

	thread->status = p9_get_thread_status(thread);

	thread->ram_is_setup = true;

	return 0;

out_fail:
	return 1;
}



static int __p9_ram_instruction(struct thread *thread, uint64_t opcode, uint64_t *scratch)
{
	uint64_t predecode, value;
	int rc;

	if (!thread->ram_is_setup)
		return 1;

	switch(opcode & OPCODE_MASK) {
	case MTNIA_OPCODE:
		opcode = 0x4c0000a4;
		opcode |= 0x001e0000;
		predecode = 8;
		break;

	case MFNIA_OPCODE:
		opcode = 0x1ac804;
		predecode = 2;
		break;

	case MTMSR_OPCODE:
		opcode |= 0x001e0000;
		predecode = 8;
		break;

	case MFSPR_OPCODE:
		switch(MXSPR_SPR(opcode)) {
		case 1: /* XER */
			predecode = 4;
			break;
		default:
			predecode = 0;
			break;
		}
		break;

	case MTSPR_OPCODE:
		switch(MXSPR_SPR(opcode)) {
		case 1: /* XER */
			predecode = 4;
			break;
		default:
			predecode = 0;
			break;
		}
		break;

	default:
		predecode = 0;
	}

	CHECK_ERR(thread_write(thread, P9_SCR0_REG, *scratch));
	value = SETFIELD(PPC_BITMASK(0, 1), 0ull, thread->id);
	value = SETFIELD(PPC_BITMASK(2, 5), value, predecode);
	value = SETFIELD(PPC_BITMASK(8, 39), value, opcode);
	CHECK_ERR(thread_write(thread, P9_RAM_CTRL, value));

	CHECK_ERR(thread_read(thread, P9_RAM_STATUS, &value));

	rc = 0;
	if (value & PPC_BIT(0)) {
		printf("Error RAMing opcode=%" PRIx64 " attempting to RAM while in recovery (status=%" PRIx64")\n", opcode, value);
		rc = 1;
		goto out;
	}
	if (value & PPC_BIT(2)) {
		printf("Error RAMing opcode=%" PRIx64 " exception or interrupt (status=%" PRIx64")\n", opcode, value);
		rc = 1;
		goto out;
	}
	if (!(value & PPC_BIT(1))) {
		printf("Warning RAMing opcode=%" PRIx64 " unexpected status=%" PRIx64"\n", opcode, value);
	}

out:
	if ((opcode & OPCODE_MASK) == LD_OPCODE) {
		while (!(value & PPC_BIT(3))) {
			CHECK_ERR(thread_read(thread, P9_RAM_STATUS, &value));
		}
	}

	if (!rc)
		CHECK_ERR(thread_read(thread, P9_SCR0_REG, scratch));

	return rc;
}

static int p9_ram_instruction(struct thread *thread, uint64_t opcode, uint64_t *scratch)
{
	if ((opcode & OPCODE_MASK) == LD_OPCODE) {
		printf("RAM LSU opcodes are disabled for POWER9 because exceptions will checkstop. Use ADU instead.\n");
		return 1;
	}

	if ((opcode & OPCODE_MASK) == LD_OPCODE) {
		/*
		 * Loads must be rammed twice, the value of the second used.
		 * A fault should still be returned though. Unfortunately
		 * any load fault seems to be a checkstop.
		 */
		int rc = __p9_ram_instruction(thread, opcode, scratch);
		if (rc)
			return rc;
	}
	return __p9_ram_instruction(thread, opcode, scratch);
}

static int p9_ram_destroy(struct thread *thread)
{
	if (!thread->ram_is_setup)
		return 1;

	/* Disable ram mode */
	CHECK_ERR(thread_write(thread, P9_RAM_MODEREG, 0));

	/* Deactivate thread for ramming */
	CHECK_ERR(thread_write(thread, P9_THREAD_INFO, 0));

	thread->status = p9_get_thread_status(thread);

	thread->ram_is_setup = false;

	return 0;
}

static int p9_ram_getxer(struct pdbg_target *thread, uint64_t *value)
{
	CHECK_ERR(thread_getspr(thread, 1, value));

	return 0;
}

static int p9_ram_putxer(struct pdbg_target *thread, uint64_t value)
{
	CHECK_ERR(thread_putspr(thread, 1, value));

	return 0;

}

static struct thread p9_thread = {
	.target = {
		.name = "POWER9 Thread",
		.compatible = "ibm,power9-thread",
		.class = "thread",
		.probe = p9_thread_probe,
		.release = p9_thread_release,
	},
	.start = p9_thread_start,
	.stop = p9_thread_stop,
	.step = p9_thread_step,
	.sreset = p9_thread_sreset,
	.ram_setup = p9_ram_setup,
	.ram_instruction = p9_ram_instruction,
	.ram_destroy = p9_ram_destroy,
	.ram_getxer = p9_ram_getxer,
	.ram_putxer = p9_ram_putxer,
};
DECLARE_HW_UNIT(p9_thread);

static int p9_core_probe(struct pdbg_target *target)
{
	struct core *core = target_to_core(target);
	int i = 0;
	uint64_t value;

	CHECK_ERR(pib_write(target, PPM_SPWKUP_FSP, PPC_BIT(0)));
	do {
		usleep(1000);
		CHECK_ERR(pib_read(target, PPM_SSHFSP, &value));

		if (i++ > SPECIAL_WKUP_TIMEOUT) {
			PR_ERROR("Timeout waiting for special wakeup on %s@0x%08" PRIx64 "\n", target->name,
				 pdbg_target_address(target, NULL));
			break;
		}
	} while (!(value & SPECIAL_WKUP_DONE));

	/* Child threads will set this to false if they are released while quiesced */
	core->release_spwkup = true;

	return 0;
}

static void p9_core_release(struct pdbg_target *target)
{
	struct pdbg_target *child;
	struct core *core = target_to_core(target);
	enum pdbg_target_status status;

	usleep(1); /* enforce small delay before and after it is cleared */

	/* Probe and release all threads to ensure release_spwkup is up to
	 * date */
	pdbg_for_each_target("thread", target, child) {
		status = pdbg_target_status(child);

		/* This thread has already been release so should have set
		 * release_spwkup to false if it was quiesced, */
		if (status == PDBG_TARGET_RELEASED)
			continue;

		status = pdbg_target_probe(child);
		if (status != PDBG_TARGET_ENABLED)
			continue;

		/* Release the thread to ensure release_spwkup is updated. */
		pdbg_target_release(child);
	}

	if (!core->release_spwkup)
		return;

	pib_write(target, PPM_SPWKUP_FSP, 0);
	usleep(10000);
}

static struct core p9_core = {
	.target = {
		.name = "POWER9 Core",
		.compatible = "ibm,power9-core",
		.class = "core",
		.probe = p9_core_probe,
		.release = p9_core_release,
	},
};
DECLARE_HW_UNIT(p9_core);

__attribute__((constructor))
static void register_p9chip(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p9_thread_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p9_core_hw_unit);
}
