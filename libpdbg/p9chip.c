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

#include "target.h"
#include "operations.h"
#include "bitutils.h"

#define P9_RAS_STATUS 0x10a02
#define P9_THREAD_INFO 0x10a9b
#define P9_DIRECT_CONTROL 0x10a9c
#define P9_RAS_MODEREG 0x10a9d
#define P9_RAM_MODEREG 0x10a4e
#define P9_RAM_CTRL 0x10a4f
#define P9_RAM_STATUS 0x10a50
#define P9_SCOMC 0x10a80
#define P9_SPR_MODE 0x10a84
#define P9_SCR0_REG 0x10a86

/* PCB Slave Registers */
#define NET_CTRL0 0xf0040
#define  NET_CTRL0_CHIPLET_ENABLE PPC_BIT(0)
#define PPM_GPMMR 0xf0100
#define PPM_SPWKUP_OTR 0xf010a
#define  SPECIAL_WKUP_DONE PPC_BIT(1)

#define RAS_STATUS_TIMEOUT	100
#define SPECIAL_WKUP_TIMEOUT	10

static uint64_t thread_read(struct thread *thread, uint64_t addr, uint64_t *data)
{
	struct pdbg_target *chip = require_target_parent(&thread->target);

	return pib_read(chip, addr, data);
}

static uint64_t thread_write(struct thread *thread, uint64_t addr, uint64_t data)
{
	struct pdbg_target *chip = require_target_parent(&thread->target);

	return pib_write(chip, addr, data);
}

static uint64_t p9_get_thread_status(struct thread *thread)
{
	uint64_t value, status = THREAD_STATUS_ACTIVE;

	thread_read(thread, P9_RAS_STATUS, &value);
	if (GETFIELD(PPC_BITMASK(8*thread->id, 3 + 8*thread->id), value) == 0xf)
		status |= THREAD_STATUS_QUIESCE;

	return status;
}

static int p9_thread_probe(struct pdbg_target *target)
{
	struct thread *thread = target_to_thread(target);

	thread->id = dt_prop_get_u32(target, "tid");
	thread->status = p9_get_thread_status(thread);

	return 0;
}

static int p9_thread_start(struct thread *thread)
{
	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(6 + 8*thread->id));
	thread_write(thread, P9_RAS_MODEREG, 0);

	return 0;
}

static int p9_thread_stop(struct thread *thread)
{
	int i = 0;

	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(7 + 8*thread->id));
	while(!(p9_get_thread_status(thread) & THREAD_STATUS_QUIESCE)) {
		if (i++ > RAS_STATUS_TIMEOUT) {
			PR_ERROR("Unable to quiesce thread\n");
			break;
		}
	}

	/* Fence interrupts. We can't do a read-modify-write here due to an
	 * errata */
	thread_write(thread, P9_RAS_MODEREG, PPC_BIT(57));

	return 0;
}

static int p9_thread_sreset(struct thread *thread)
{
	/* Can only sreset if a thread is inactive, at least on DD1 */
	if (p9_get_thread_status(thread) != (THREAD_STATUS_QUIESCE | THREAD_STATUS_ACTIVE))
		return 1;

	/* This will force SRR1[46:47] == 0b00 which means the kernel should
	 * enter xmon. However it will hide the fact we may have come from a
	 * powersave state in which register contents were lost. We need a
	 * kernel side fix for that. */
	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(32 + 8*thread->id));
	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(4 + 8*thread->id));

	return 0;
}

static int p9_ram_setup(struct thread *thread)
{
	struct pdbg_target *target;
	struct core *chip = target_to_core(thread->target.parent);

	/* We can only ram a thread if all the threads on the core/chip are
	 * quiesced */
	dt_for_each_compatible(&chip->target, target, "ibm,power9-thread") {
		struct thread *tmp;

		/* If this thread wasn't enabled it may not yet have been probed
		   so do that now. This will also update the thread status */
		p9_thread_probe(target);
		tmp = target_to_thread(target);
		if (tmp->status != (THREAD_STATUS_QUIESCE | THREAD_STATUS_ACTIVE))
			return 1;
	}

 	/* Enable ram mode */
	CHECK_ERR(thread_write(thread, P9_RAM_MODEREG, PPC_BIT(0)));

	/* Setup SPRC to use SPRD */
	CHECK_ERR(thread_write(thread, P9_SPR_MODE, 0x00000ff000000000));
	CHECK_ERR(thread_write(thread, P9_SCOMC, 0x0));

	return 0;
}

static int p9_ram_instruction(struct thread *thread, uint64_t opcode, uint64_t *scratch)
{
	uint64_t predecode, value;

	switch(opcode & OPCODE_MASK) {
	case MTNIA_OPCODE:
		predecode = 8;

		/* Not currently supported as we can only MTNIA from LR */
		PR_ERROR("MTNIA is not currently supported\n");
		break;

	case MFNIA_OPCODE:
		opcode = 0x1ac804;
		predecode = 2;
		break;

	case MTMSR_OPCODE:
		predecode = 8;
		break;

	default:
		predecode = 0;
	}

	CHECK_ERR(thread_write(thread, P9_SCR0_REG, *scratch));
	value = SETFIELD(PPC_BITMASK(0, 1), 0ull, thread->id);
	value = SETFIELD(PPC_BITMASK(2, 5), value, predecode);
	value = SETFIELD(PPC_BITMASK(8, 39), value, opcode);
	CHECK_ERR(thread_write(thread, P9_RAM_CTRL, value));
	do {
		CHECK_ERR(thread_read(thread, P9_RAM_STATUS, &value));
		if (((value & PPC_BIT(0)) || (value & PPC_BIT(2))))
			return 1;
	} while (!(value & PPC_BIT(1) && !(value & PPC_BIT(3))));
	CHECK_ERR(thread_read(thread, P9_SCR0_REG, scratch));

	return 0;
}

static int p9_ram_destroy(struct thread *thread)
{
	/* Disable ram mode */
	CHECK_ERR(thread_write(thread, P9_RAM_MODEREG, 0));

	return 0;
}

struct thread p9_thread = {
	.target = {
		.name = "POWER9 Thread",
		.compatible = "ibm,power9-thread",
		.class = "thread",
		.probe = p9_thread_probe,
	},
	.start = p9_thread_start,
	.stop = p9_thread_stop,
	.sreset = p9_thread_sreset,
	.ram_setup = p9_ram_setup,
	.ram_instruction = p9_ram_instruction,
	.ram_destroy = p9_ram_destroy,
};
DECLARE_HW_UNIT(p9_thread);

static int p9_core_probe(struct pdbg_target *target)
{
	int i = 0;
	uint64_t value;

	if (pib_read(target, NET_CTRL0, &value))
		return -1;

	if (!(value & NET_CTRL0_CHIPLET_ENABLE))
		return -1;

	CHECK_ERR(pib_write(target, PPM_SPWKUP_OTR, PPC_BIT(0)));
	do {
		usleep(1);
		CHECK_ERR(pib_read(target, PPM_GPMMR, &value));

		if (i++ > SPECIAL_WKUP_TIMEOUT) {
			PR_ERROR("Timeout waiting for special wakeup on %s@0x%08" PRIx64 "\n", target->name,
				 dt_get_address(target, 0, NULL));
			break;
		}
	} while (!(value & SPECIAL_WKUP_DONE));

	return 0;
}

struct core p9_core = {
	.target = {
		.name = "POWER9 Core",
		.compatible = "ibm,power9-core",
		.class = "core",
		.probe = p9_core_probe,
	},
};
DECLARE_HW_UNIT(p9_core);

static int p9_chiplet_probe(struct pdbg_target *target)
{
        uint64_t value;

        if (pib_read(target, NET_CTRL0, &value))
                return -1;

        if (!(value & NET_CTRL0_CHIPLET_ENABLE))
                return -1;

        return 0;
}

struct chiplet p9_chiplet = {
        .target = {
                .name = "POWER9 Chiplet",
                .compatible = "ibm,power9-chiplet",
                .class = "chiplet",
                .probe = p9_chiplet_probe,
        },
};
DECLARE_HW_UNIT(p9_chiplet);
