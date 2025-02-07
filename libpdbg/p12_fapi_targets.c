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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "hwunit.h"
#include "bitutils.h"
//TODO : need to work on p12 scom addr translation
#include "p12_scom_addr.h"

#define t(x) (&(x)->target)

static uint64_t p12_eq_translate(struct eq *eq, uint64_t addr)
{
	addr = set_chiplet_id(addr, EQ0_CHIPLET_ID + pdbg_target_index(t(eq)));

	return addr;
}

static struct eq p12_eq = {
	.target = {
		.name = "POWER12 eq",
		.compatible = "ibm,power12-eq",
		.class = "eq",
		.translate = translate_cast(p12_eq_translate),
	},
};
DECLARE_HW_UNIT(p12_eq);

#define HEADER_CHECK_DATA ((uint64_t) 0xc0ffee03 << 32)

static int p12_chiplet_getring(struct chiplet *chiplet, uint64_t ring_addr, int64_t ring_len, uint32_t result[])
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

#define NET_CTRL0	0xf0040
#define  NET_CTRL0_CHIPLET_ENABLE	PPC_BIT(0)
static int p12_chiplet_probe(struct pdbg_target *target)
{
	uint64_t value;

	if (pib_read(target, NET_CTRL0, &value))
		return -1;

	if (!(value & NET_CTRL0_CHIPLET_ENABLE))
		return -1;

	return 0;
}

static uint64_t p12_chiplet_translate(struct chiplet *chiplet, uint64_t addr)
{
	return set_chiplet_id(addr, pdbg_target_index(t(chiplet)));
}

static struct chiplet p12_chiplet = {
	.target = {
		.name = "POWER10 Chiplet",
		.compatible = "ibm,power12-chiplet",
		.class = "chiplet",
		.probe = p12_chiplet_probe,
		.translate = translate_cast(p12_chiplet_translate),
	},
	.getring = p12_chiplet_getring,
};
DECLARE_HW_UNIT(p12_chiplet);

static uint64_t no_translate(struct pdbg_target *target, uint64_t addr)
{
	/*  No translation performed */
	return 0;
}

static struct fc p12_fc = {
	.target = {
		.name = "POWER10 Fused Core",
		.compatible = "ibm,power12-fc",
		.class = "fc",
		.translate = no_translate,
	},
};
DECLARE_HW_UNIT(p12_fc);

static struct tbusc p12_tbusc = {
	.target = {
		.name = "tbusc",
		.compatible = "ibm,power12-tbusc",
		.class = "tbusc",
		.translate = no_translate,
	},
};
DECLARE_HW_UNIT(p12_tbusc);

static struct tbusl p12_tbusl = {
	.target = {
		.name = "tbusl",
		.compatible = "ibm,power12-tbus1",
		.class = "tbusl",
		.translate = no_translate,
	},
};
DECLARE_HW_UNIT(p12_tbusl);

static struct l3cache p12_l3cache = {
	.target = {
		.name = "l3cache",
		.compatible = "ibm,power12-l3cache",
		.class = "l3cache",
		.translate = no_translate,
	},
};
DECLARE_HW_UNIT(p12_l3cache);

__attribute__((constructor))
static void register_2_fapi_targets(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_eq_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_chiplet_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_fc_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_tbusl_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_tbusc_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p12_l3cache_hw_unit);
}
