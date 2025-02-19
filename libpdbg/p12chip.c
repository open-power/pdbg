/* Copyright 2020 IBM Corp.
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
#include <inttypes.h>

#include "hwunit.h"
#include "bitutils.h"
#include "operations.h"
#include "chip.h"
#include "debug.h"

/*
 * NOTE!
 * All timeouts and scom procedures in general through the file should be kept
 * in synch with skiboot (e.g., core/direct-controls.c) as far as possible.
 * If you fix a bug here, fix it in skiboot, and vice versa.
 */

/* PCB Slave registers */
#define QME_SSH_FSP		0xE8824
#define  SPECIAL_WKUP_DONE	PPC_BIT(1)
#define QME_SPWU_FSP		0xE8834

#define RAS_STATUS_TIMEOUT	100 /* 100ms */
#define SPECIAL_WKUP_TIMEOUT	100 /* 100ms */

static int p12_core_probe(struct pdbg_target *target)
{
	struct core *core = target_to_core(target);
	uint64_t value;
	int i = 0;

	/*
	 * BMC applications using libpdbg, do not need special wakeup
	 * asserted by default. Only when running pdbg tool or equivalent
	 * assert special wakeup.
	 */
	if (!pdbg_context_is_short()) {
		core->release_spwkup = false;
		return 0;
	}

	CHECK_ERR(pib_write(target, QME_SPWU_FSP, PPC_BIT(0)));
	do {
		usleep(1000);
		CHECK_ERR(pib_read(target, QME_SSH_FSP, &value));

		if (i++ > SPECIAL_WKUP_TIMEOUT) {
			PR_ERROR("Timeout waiting for special wakeup on %s\n",
				 pdbg_target_path(target));
			break;
		}
	} while (!(value & SPECIAL_WKUP_DONE));

	core->release_spwkup = true;

	return 0;
}

static void p12_core_release(struct pdbg_target *target)
{
	struct core *core = target_to_core(target);
	
	/* Probe and release all threads to ensure release_spwkup is up to
	 * date */
	/* TODO Do we need thread?*/

	if (!core->release_spwkup)
		return;

	pib_write(target, QME_SPWU_FSP, 0);
}

#define NUM_CORES_PER_EQ 4
#define EQ0_CHIPLET_ID 0x20

static uint64_t p12_core_translate(struct core *c, uint64_t addr)
{
	int region = 0;
	int chip_unitnum = pdbg_target_index(&c->target);
	int chiplet_id = EQ0_CHIPLET_ID + chip_unitnum / NUM_CORES_PER_EQ;

	switch(chip_unitnum % NUM_CORES_PER_EQ) {
	case 0:
		region = 8;
		break;
	case 1:
		region = 4;
		break;
	case 2:
		region = 2;
		break;
	case 3:
		region = 1;
		break;
	}
	addr &= 0xFFFFFFFFC0FFFFFFULL;
	addr |= ((chiplet_id & 0x3F) << 24);

	addr &= 0xFFFFFFFFFFFF0FFFULL;
	addr |= ((region & 0xF) << 12);

	return addr;
}

static struct core p12_core = {
	.target = {
		.name = "POWER12 Core",
		.compatible = "ibm,power12-core",
		.class = "core",
		.probe = p12_core_probe,
		.release = p12_core_release,
		.translate = translate_cast(p12_core_translate),
	},
};
DECLARE_HW_UNIT(p12_core);

__attribute__((constructor))
static void register_p12chip(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_core_hw_unit);
}