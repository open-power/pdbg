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

#define NUM_CORES_PER_EQ 4
#define EQ0_CHIPLET_ID 0x20

static uint64_t p10_core_translate(struct core *c, uint64_t addr)
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

static struct core p10_core = {
	.target = {
		.name = "POWER10 Core",
		.compatible = "ibm,power10-core",
		.class = "core",
		.translate = translate_cast(p10_core_translate),
	},
};
DECLARE_HW_UNIT(p10_core);

__attribute__((constructor))
static void register_p10chip(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_core_hw_unit);
}
