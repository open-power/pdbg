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
#include "p10_scom_addr.h"

#define t(x) (&(x)->target)

static uint64_t p10_eq_translate(struct eq *eq, uint64_t addr)
{
	addr = set_chiplet_id(addr, EQ0_CHIPLET_ID + pdbg_target_index(t(eq)));

	return addr;
}

static struct eq p10_eq = {
	.target = {
		.name = "POWER10 eq",
		.compatible = "ibm,power10-eq",
		.class = "eq",
		.translate = translate_cast(p10_eq_translate),
	},
};
DECLARE_HW_UNIT(p10_eq);

static uint64_t p10_pec_translate(struct pec *pec, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(pec));

	if (get_chiplet_id(addr) >= N0_CHIPLET_ID &&
	    get_chiplet_id(addr) <= N1_CHIPLET_ID)
		return set_chiplet_id(addr,
				      chip_unitnum ? N0_CHIPLET_ID : N1_CHIPLET_ID );
	else
		return set_chiplet_id(addr, PCI0_CHIPLET_ID + pdbg_target_index(t(pec)));
}

static struct pec p10_pec = {
	.target = {
		.name = "POWER10 pec",
		.compatible = "ibm,power10-pec",
		.class = "pec",
		.translate = translate_cast(p10_pec_translate),
	},
};
DECLARE_HW_UNIT(p10_pec);

static uint64_t p10_phb_translate(struct phb *phb, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(phb));

	if (get_chiplet_id(addr) >= N0_CHIPLET_ID && get_chiplet_id(addr) <= N1_CHIPLET_ID) {
		addr = set_chiplet_id(addr, chip_unitnum / 3 ? N0_CHIPLET_ID : N1_CHIPLET_ID);
		return set_sat_id(addr, 1 + chip_unitnum % 3);
	} else {
		addr = set_chiplet_id(addr, chip_unitnum / 3 + PCI0_CHIPLET_ID);
		if (get_ring_id(addr) == 2) {
			if (get_sat_id(addr) >= 1 && get_sat_id(addr) <= 3)
				return set_sat_id(addr, 1 + chip_unitnum % 3);
			else
				return set_sat_id(addr, 4 + chip_unitnum % 3);
		}
	}

	/*  We'll never get here due to the assert but gcc complains */
	return addr;
}

static struct phb p10_phb = {
	.target = {
		.name = "POWER10 phb",
		.compatible = "ibm,power10-phb",
		.class = "phb",
		.translate = translate_cast(p10_phb_translate),
	},
};
DECLARE_HW_UNIT(p10_phb);

static uint64_t p10_nmmu_translate(struct nmmu *nmmu, uint64_t addr)
{
	return set_chiplet_id(addr, pdbg_target_index(t(nmmu)) + N0_CHIPLET_ID);
}

static struct nmmu p10_nmmu = {
	.target = {
		.name = "POWER10 nmmu",
		.compatible = "ibm,power10-nmmu",
		.class = "nmmu",
		.translate = translate_cast(p10_nmmu_translate),
	},
};
DECLARE_HW_UNIT(p10_nmmu);

static uint64_t p10_iohs_translate(struct iohs *iohs, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(iohs));

	if (get_chiplet_id(addr) >= AXON0_CHIPLET_ID &&
	    get_chiplet_id(addr) <= AXON7_CHIPLET_ID)
		addr = set_chiplet_id(addr, AXON0_CHIPLET_ID + chip_unitnum);
	else if (get_chiplet_id(addr) >= PAU0_CHIPLET_ID &&
		 get_chiplet_id(addr) <= PAU3_CHIPLET_ID) {
		addr = set_chiplet_id(addr, chip_unitnum/2 + PAU0_CHIPLET_ID);
	} else
		/* We should bail here with an assert but it makes testing hard and we
		 * should never hit it anyway as all code will have been validated
		 * through the EKB CI process (LOL). */
		assert(1);

	if (get_chiplet_id(addr) >= PAU0_CHIPLET_ID &&
	    get_chiplet_id(addr) <= PAU3_CHIPLET_ID) {
		if (chip_unitnum % 2)
			addr = set_io_group_addr(addr, 0x1);
		else
			addr = set_io_group_addr(addr, 0x0);
	}

	return addr;
}

static struct iohs p10_iohs = {
	.target = {
		.name = "POWER10 iohs",
		.compatible = "ibm,power10-iohs",
		.class = "iohs",
		.translate = translate_cast(p10_iohs_translate),
	},
};
DECLARE_HW_UNIT(p10_iohs);

/* We take a struct pdbg_target and avoid the casting here as the translation is
 * the same for both target types. */
static uint64_t p10_mimc_translate(struct pdbg_target *mimc, uint64_t addr)
{
	return set_chiplet_id(addr, pdbg_target_index(mimc) + MC0_CHIPLET_ID);
}

static struct mi p10_mi = {
	.target = {
		.name = "POWER10 mi",
		.compatible = "ibm,power10-mi",
		.class = "mi",
		.translate = p10_mimc_translate,
	},
};
DECLARE_HW_UNIT(p10_mi);

static struct mc p10_mc = {
	.target = {
		.name = "POWER10 mc",
		.compatible = "ibm,power10-mc",
		.class = "mc",
		.translate = p10_mimc_translate,
	},
};
DECLARE_HW_UNIT(p10_mc);

static uint64_t p10_mcc_translate(struct mcc *mcc, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(mcc));
	uint8_t offset = get_sat_offset(addr);

	addr = set_chiplet_id(addr, chip_unitnum/2 + MC0_CHIPLET_ID);
	if (chip_unitnum % 2) {
		switch (get_sat_id(addr)) {
		case 0x4:
			addr = set_sat_id(addr, 0x5);
			break;
		case 0x8:
			addr = set_sat_id(addr, 0x9);
			break;
		case 0x0:
			if (offset >= 0x22 && offset <= 0x2b)
				addr = set_sat_offset(addr, offset + 0x10);
			break;
		case 0xd:
			if (offset >= 0x00 && offset <= 0x1f)
				addr = set_sat_offset(addr, offset + 0x20);
			break;
		}
	} else {
		switch (get_sat_id(addr)) {
		case 0x5:
			addr = set_sat_id(addr, 0x4);
			break;
		case 0x9:
			addr = set_sat_id(addr, 0x8);
			break;
		case 0x0:
			if (offset >= 0x32 && offset <= 0x3b)
				addr = set_sat_offset(addr, offset - 0x10);
			break;
		case 0xd:
			if (offset >= 0x20 && offset <= 0x3f)
				addr = set_sat_offset(addr, offset - 0x20);
			break;
		}
	}

	return addr;
}

static struct mcc p10_mcc = {
	.target = {
		.name = "POWER10 mcc",
		.compatible = "ibm,power10-mcc",
		.class = "mcc",
		.translate = translate_cast(p10_mcc_translate),
	},
};
DECLARE_HW_UNIT(p10_mcc);

static uint64_t p10_omic_translate(struct omic *omic, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(omic));
	int chiplet_id = get_chiplet_id(addr);

	if (chiplet_id >= PAU0_CHIPLET_ID && chiplet_id <= PAU3_CHIPLET_ID) {
		if (chip_unitnum == 0 || chip_unitnum == 1)
			addr = set_chiplet_id(addr, PAU0_CHIPLET_ID);
		else if (chip_unitnum == 2 || chip_unitnum == 3)
			addr = set_chiplet_id(addr, PAU2_CHIPLET_ID);
		else if (chip_unitnum == 4 || chip_unitnum == 5)
			addr = set_chiplet_id(addr, PAU1_CHIPLET_ID);
		else if (chip_unitnum == 6 || chip_unitnum == 7)
			addr = set_chiplet_id(addr, PAU3_CHIPLET_ID);
		else
			assert(0);

		if (chip_unitnum % 2)
			addr = set_io_group_addr(addr, 0x3);
		else
			addr = set_io_group_addr(addr, 0x2);
	} else {
		addr = set_chiplet_id(addr, chip_unitnum/2 + MC0_CHIPLET_ID);

		if (chip_unitnum % 2)
			addr = set_ring_id(addr, 0x6);
		else
			addr = set_ring_id(addr, 0x5);
	}

	return addr;
}

static struct omic p10_omic = {
	.target = {
		.name = "POWER10 omic",
		.compatible = "ibm,power10-omic",
		.class = "omic",
		.translate = translate_cast(p10_omic_translate),
	},
};
DECLARE_HW_UNIT(p10_omic);

static uint64_t p10_omi_translate(struct omi *omi, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(omi));
	int chiplet_id = get_chiplet_id(addr);

	if (chiplet_id >= PAU0_CHIPLET_ID && chiplet_id <= PAU3_CHIPLET_ID) {
		if (chip_unitnum >= 0 && chip_unitnum <= 3)
			addr = set_chiplet_id(addr, PAU0_CHIPLET_ID);
		else if (chip_unitnum >= 4 && chip_unitnum <= 7)
			addr = set_chiplet_id(addr, PAU2_CHIPLET_ID);
		else if (chip_unitnum >= 8 && chip_unitnum <= 11)
			addr = set_chiplet_id(addr, PAU1_CHIPLET_ID);
		else if (chip_unitnum >= 12 && chip_unitnum <= 15)
			addr = set_chiplet_id(addr, PAU3_CHIPLET_ID);
		else
			assert(0);

		if (chip_unitnum % 2)
			addr = set_io_lane(addr, 8 + get_io_lane(addr) % 8);
		else
			addr = set_io_lane(addr, get_io_lane(addr) % 8);

		if ((chip_unitnum / 2) % 2)
			addr = set_io_group_addr(addr, 0x3);
		else
			addr = set_io_group_addr(addr, 0x2);
	} else {
		addr = set_chiplet_id(addr, chip_unitnum/4 + MC0_CHIPLET_ID);

		if (get_sat_offset(addr) >= 16 && get_sat_offset(addr) <= 47) {
			if (chip_unitnum % 2)
				addr = set_sat_offset(addr, 32 + get_sat_offset(addr) % 16);
			else
				addr = set_sat_offset(addr, 16 + get_sat_offset(addr) % 16);
		} else {
			if (chip_unitnum % 2)
				addr = set_sat_offset(addr, 56 + get_sat_offset(addr) % 4);
			else
				addr = set_sat_offset(addr, 48 + get_sat_offset(addr) % 4);
		}

		if ((chip_unitnum / 2) %2)
			addr = set_ring_id(addr, 0x6);
		else
			addr = set_ring_id(addr, 0x5);
	}

	return addr;
}

static struct omi p10_omi = {
	.target = {
		.name = "POWER10 omi",
		.compatible = "ibm,power10-omi",
		.class = "omi",
		.translate = translate_cast(p10_omi_translate),
	},
};
DECLARE_HW_UNIT(p10_omi);

static uint64_t p10_pauc_translate(struct pauc *pauc, uint64_t addr)
{
	return set_chiplet_id(addr, pdbg_target_index(t(pauc)) + PAU0_CHIPLET_ID);
}

static struct pauc p10_pauc = {
	.target = {
		.name = "POWER10 pauc",
		.compatible = "ibm,power10-pauc",
		.class = "pauc",
		.translate = translate_cast(p10_pauc_translate),
	},
};
DECLARE_HW_UNIT(p10_pauc);

static uint64_t p10_pau_translate(struct pau *pau, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(pau));

	addr = set_chiplet_id(addr, chip_unitnum/2 + PAU0_CHIPLET_ID);

	switch (chip_unitnum) {
	case 0:
	case 3:
	case 4:
	case 6:
		if (get_ring_id(addr) == 0x4)
			addr = set_ring_id(addr, 0x2);
		else if (get_ring_id(addr) == 0x5)
			addr = set_ring_id(addr, 0x3);
		break;

	case 1:
	case 2:
	case 5:
	case 7:
		if (get_ring_id(addr) == 0x2)
			addr = set_ring_id(addr, 0x4);
		else if (get_ring_id(addr) == 0x3)
			addr = set_ring_id(addr, 0x5);
		break;
	}

	return addr;
}

static struct pau p10_pau = {
	.target = {
		.name = "POWER10 pau",
		.compatible = "ibm,power10-pau",
		.class = "pau",
		.translate = translate_cast(p10_pau_translate),
	},
};
DECLARE_HW_UNIT(p10_pau);

#define HEADER_CHECK_DATA ((uint64_t) 0xc0ffee03 << 32)

static int p10_chiplet_getring(struct chiplet *chiplet, uint64_t ring_addr, int64_t ring_len, uint32_t result[])
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
static int p10_chiplet_probe(struct pdbg_target *target)
{
	uint64_t value;

	if (pib_read(target, NET_CTRL0, &value))
		return -1;

	if (!(value & NET_CTRL0_CHIPLET_ENABLE))
		return -1;

	return 0;
}

static uint64_t p10_chiplet_translate(struct chiplet *chiplet, uint64_t addr)
{
	return set_chiplet_id(addr, pdbg_target_index(t(chiplet)));
}

static struct chiplet p10_chiplet = {
	.target = {
		.name = "POWER10 Chiplet",
		.compatible = "ibm,power10-chiplet",
		.class = "chiplet",
		.probe = p10_chiplet_probe,
		.translate = translate_cast(p10_chiplet_translate),
	},
	.getring = p10_chiplet_getring,
};
DECLARE_HW_UNIT(p10_chiplet);

static uint64_t no_translate(struct pdbg_target *target, uint64_t addr)
{
	/*  No translation performed */
	return 0;
}

static struct fc p10_fc = {
	.target = {
		.name = "POWER10 Fused Core",
		.compatible = "ibm,power10-fc",
		.class = "fc",
		.translate = no_translate,
	},
};
DECLARE_HW_UNIT(p10_fc);

static struct smpgroup p10_smpgroup = {
	.target = {
		.name = "POWER10 SMP Group",
		.compatible = "ibm,power10-smpgroup",
		.class = "smpgroup",
		.translate = no_translate,
	},
};
DECLARE_HW_UNIT(p10_smpgroup);

static struct dimm p10_dimm = {
    .target = {
        .name = "POWER10 DIMM",
        .compatible = "ibm,power10-dimm",
        .class = "dimm",
        .translate = no_translate,
    },
};
DECLARE_HW_UNIT(p10_dimm);

struct mem_port p10_mem_port = {
    .target = {
        .name = "POWER10 mem_port",
        .compatible = "ibm,power10-memport",
        .class = "mem_port",
        .translate = no_translate,
    },
};
DECLARE_HW_UNIT(p10_mem_port);

__attribute__((constructor))
static void register_p10_fapi_targets(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_eq_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_pec_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_phb_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_nmmu_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_pau_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_iohs_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_mi_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_mc_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_mcc_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_omic_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_omi_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_pauc_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_pau_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_chiplet_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_fc_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_smpgroup_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_mem_port_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_dimm_hw_unit);
}
