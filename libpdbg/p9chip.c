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
#define NET_CTRL0	0xf0040
#define  NET_CTRL0_CHIPLET_ENABLE	PPC_BIT(0)
#define  NET_CTRL0_FENCE_EN 		PPC_BIT(18)
#define NET_CTRL0_WOR	0xf0042
#define PPM_GPMMR	0xf0100
#define PPM_SPWKUP_OTR	0xf010a
#define PPM_SSHOTR	0xf0113
#define  SPECIAL_WKUP_DONE PPC_BIT(1)

#define RAS_STATUS_TIMEOUT	100 /* 100ms */
#define SPECIAL_WKUP_TIMEOUT	100 /* 100ms */

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
	uint64_t value, status = 0;

	thread_read(thread, P9_RAS_STATUS, &value);
	if (GETFIELD(PPC_BITMASK(8*thread->id, 3 + 8*thread->id), value) == 0xf)
		status |= THREAD_STATUS_QUIESCE;

	thread_read(thread, P9_THREAD_INFO, &value);
	if (value & PPC_BIT(thread->id))
		status |= THREAD_STATUS_ACTIVE;

	thread_read(thread, P9_CORE_THREAD_STATE, &value);
	if (value & PPC_BIT(56 + thread->id))
		status |= THREAD_STATUS_STOP;

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
	if (!(thread->status & THREAD_STATUS_QUIESCE))
		return 1;

	if ((!(thread->status & THREAD_STATUS_ACTIVE)) ||
	    (thread->status & THREAD_STATUS_STOP)) {
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
	while (!(p9_get_thread_status(thread) & THREAD_STATUS_QUIESCE)) {
		usleep(1000);
		if (i++ > RAS_STATUS_TIMEOUT) {
			PR_ERROR("Unable to quiesce thread\n");
			break;
		}
	}
	thread->status = p9_get_thread_status(thread);

	return 0;
}

static int p9_thread_sreset(struct thread *thread)
{
	/* Can only sreset if a thread is inactive */
	if (!(thread->status & THREAD_STATUS_QUIESCE))
		return 1;

	thread_write(thread, P9_DIRECT_CONTROL, PPC_BIT(4 + 8*thread->id));

	thread->status = p9_get_thread_status(thread);

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
		if (!(tmp->status & THREAD_STATUS_QUIESCE))
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

#define HEADER_CHECK_DATA ((uint64_t) 0xc0ffee03 << 32)

static int p9_chiplet_getring(struct chiplet *chiplet, uint64_t ring_addr, int64_t ring_len, uint32_t result[])
{
	uint64_t scan_type_addr;
	uint64_t scan_data_addr;
	uint64_t scan_header_addr;
	uint64_t scan_type_data;
	uint64_t set_pulse = 1;
	uint64_t bits = 32;
	uint64_t data;

	/* We skip the first word in the results so we can write it later as it
	 * should contain the header read out at the end */
	int i = 0;

	scan_type_addr = (ring_addr & 0x7fff0000) | 0x7;
	scan_data_addr = (scan_type_addr & 0xffff0000) | 0x8000;
	scan_header_addr = scan_data_addr & 0xffffe000;

	scan_type_data = (ring_addr & 0xfff0) << 13;
	scan_type_data |= 0x800 >> (ring_addr & 0xf);
	scan_type_data <<= 32;

	pib_write(&chiplet->target, scan_type_addr, scan_type_data);
	pib_write(&chiplet->target, scan_header_addr, HEADER_CHECK_DATA);

	/* The final 32 bit read is the header which we do at the end */
	ring_len -= 32;
	i = 1;

	while (ring_len > 0) {
		ring_len -= bits;
		if (set_pulse) {
			scan_data_addr |= 0x4000;
			set_pulse = 0;
		} else
			scan_data_addr &= ~0x4000ULL;

		scan_data_addr &= ~0xffull;
		scan_data_addr |= bits;
		pib_read(&chiplet->target, scan_data_addr, &data);

		/* Discard lower 32 bits */
		/* TODO: We always read 64-bits from the ring on P9 so we could
		 * optimise here by reading 64-bits at a time, but I'm not
		 * confident I've figured that out and 32-bits is what Hostboot
		 * does and seems to work. */
		data >>= 32;

		/* Left-align data */
		data <<= 32 - bits;
		result[i++] = data;
		if (ring_len > 0 && (ring_len < bits))
			bits = ring_len;
	}

	pib_read(&chiplet->target, scan_header_addr | 0x20, &data);
	data &= 0xffffffff00000000;
	result[0] = data >> 32;
	if (data != HEADER_CHECK_DATA)
		printf("WARNING: Header check failed. Make sure you specified the right ring length!\n"
		       "Ring data is probably corrupt now.\n");

	return 0;
}

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
		usleep(1000);
		CHECK_ERR(pib_read(target, PPM_SSHOTR, &value));

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
	.getring = p9_chiplet_getring,
};
DECLARE_HW_UNIT(p9_chiplet);
