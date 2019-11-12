/* Copyright 2018 IBM Corp.
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
#include "p9_scom_addr.h"

#define t(x) (&(x)->target)

/* There is a FAPI target type but no scominfo, not sure how to do
 * translation in this case so just explode. */
static uint64_t p9_unknown_translation(struct pdbg_target *target, uint64_t addr)
{
	pdbg_log(PDBG_ERROR, "Translation of address 0x%016" PRIx64 " unknown for target %s\n",
		 addr, pdbg_target_path(target));
	return -1;
}

static uint64_t p9_ex_translate(struct ex *ex, uint64_t addr)
{
	if (get_chiplet_id(addr) >= EP00_CHIPLET_ID &&
	    get_chiplet_id(addr) <= EP05_CHIPLET_ID) {
		uint8_t ring_id = get_ring(addr) & 0xf;
		addr = set_chiplet_id(addr, EP00_CHIPLET_ID + pdbg_target_index(t(ex)) / 2);
		ring_id = ring_id - ring_id % 2 + pdbg_target_index(t(ex)) % 2;
		return set_ring(addr, ring_id & 0xf);
	} else if (get_chiplet_id(addr) >= EC00_CHIPLET_ID &&
		   get_chiplet_id(addr) <= EC23_CHIPLET_ID) {
		return set_chiplet_id(addr, EC00_CHIPLET_ID +
				      get_chiplet_id(addr) % 2 +
				      pdbg_target_index(t(ex)) * 2);
	} else {
		pdbg_log(PDBG_WARNING, "Invalid ex address 0x%016" PRIx64 "\n", addr);
		return addr;
	}
}

struct ex p9_ex = {
        .target = {
                .name = "POWER9 ex",
                .compatible = "ibm,power9-ex",
                .class = "ex",
		.translate = translate_cast(p9_ex_translate),
        },
};
DECLARE_HW_UNIT(p9_ex);

struct mba p9_mba = {
        .target = {
                .name = "POWER9 mba",
                .compatible = "ibm,power9-mba",
                .class = "mba",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_mba);

static uint64_t p9_mcs_translate(struct mcs *mcs, uint64_t addr)
{
	uint32_t chiplet = N3_CHIPLET_ID - 2 * (pdbg_target_index(t(mcs)) / 2);
	uint32_t sat = 2 * (pdbg_target_index(t(mcs)) % 2);

	addr = set_chiplet_id(addr, chiplet);
	addr = set_sat_id(addr, sat);

	return addr;
}

struct mcs p9_mcs = {
        .target = {
                .name = "POWER9 mcs",
                .compatible = "ibm,power9-mcs",
                .class = "mcs",
		.translate = translate_cast(p9_mcs_translate),
        },
};
DECLARE_HW_UNIT(p9_mcs);

/* XBus addressing is more complicated. This comes from p9_scominfo.C
 * in the ekb. */
static uint64_t p9_xbus_translate(struct xbus *xbus, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(xbus));
	uint64_t ring = get_ring(addr) & 0xf;

	if (ring >= XB_IOX_0_RING_ID &&
	    ring <= XB_IOX_2_RING_ID)
		return set_ring(addr, (XB_IOX_0_RING_ID + chip_unitnum) & 0xf);
	else if (ring >= XB_PBIOX_0_RING_ID &&
		 ring <= XB_PBIOX_2_RING_ID)
		return set_ring(addr, (XB_PBIOX_0_RING_ID + chip_unitnum) & 0xf);
	else {
		pdbg_log(PDBG_WARNING, "Invalid xbus address 0x%016" PRIx64 "\n", addr);
		return addr;
	}
}

static int p9_xbus_probe(struct pdbg_target *target)
{
	struct xbus *xbus = target_to_xbus(target);

	if (pdbg_target_u32_property(&xbus->target, "ring-id", &xbus->ring_id)) {
		printf("Unknown ring-id on %s@%d\n", pdbg_target_name(&xbus->target),
		       pdbg_target_index(&xbus->target));
		return -1;
	}

	return 0;
}

struct xbus p9_xbus = {
        .target = {
                .name = "POWER9 xbus",
                .compatible = "ibm,power9-xbus",
                .class = "xbus",
		.probe = p9_xbus_probe,
		.translate = translate_cast(p9_xbus_translate),
        },
};
DECLARE_HW_UNIT(p9_xbus);

struct abus p9_abus = {
        .target = {
                .name = "POWER9 abus",
                .compatible = "ibm,power9-abus",
                .class = "abus",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_abus);

struct l4 p9_l4 = {
        .target = {
                .name = "POWER9 l4",
                .compatible = "ibm,power9-l4",
                .class = "l4",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_l4);

/* EQ targets just get translated based on the chiplet they fall
 * under */
struct eq p9_eq = {
        .target = {
                .name = "POWER9 eq",
                .compatible = "ibm,power9-eq",
                .class = "eq",
        },
};
DECLARE_HW_UNIT(p9_eq);

static uint64_t p9_mca_translate(struct mca *mca, uint64_t addr)
{
	int chiplet_unitnum = pdbg_target_index(t(mca));

	if (get_chiplet_id(addr) == MC01_CHIPLET_ID ||
	    get_chiplet_id(addr) ==  MC23_CHIPLET_ID) {
		addr = set_chiplet_id(addr, MC01_CHIPLET_ID + chiplet_unitnum / 4);
		if ((get_ring(addr) & 0xf) == MC_MC01_0_RING_ID)
			/* mc */
			return set_sat_id(addr, get_sat_id(addr) - get_sat_id(addr) % 4 +
					  chiplet_unitnum % 4);
		else
			/* iomc */
			return set_ring(addr, (MC_IOM01_0_RING_ID + chiplet_unitnum % 4) & 0xf);
	} else {
		//mcs->mca regisers
		uint8_t mcs_unitnum = chiplet_unitnum / 2;
		uint8_t mcs_sat_offset = get_sat_offset(addr) & 0x2f;

		addr = set_chiplet_id(addr, N3_CHIPLET_ID - 2 * (mcs_unitnum / 2));
		addr = set_sat_id(addr, 2 * (mcs_unitnum % 2));

		mcs_sat_offset |= (chiplet_unitnum % 2) << 4;
		addr = set_sat_offset(addr, mcs_sat_offset);

		return addr;
	}
}

struct mca p9_mca = {
        .target = {
                .name = "POWER9 mca",
                .compatible = "ibm,power9-mca",
                .class = "mca",
		.translate = translate_cast(p9_mca_translate),
        },
};
DECLARE_HW_UNIT(p9_mca);

struct mcbist p9_mcbist = {
        .target = {
                .name = "POWER9 mcbist",
                .compatible = "ibm,power9-mcbist",
                .class = "mcbist",
        },
};
DECLARE_HW_UNIT(p9_mcbist);

/* TODO: This could be modelled directly under the N1/N3 chiplet */
static uint64_t p9_mi_translate(struct mi *mi, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(mi));

	addr = set_chiplet_id(addr, N3_CHIPLET_ID - 2 * (chip_unitnum / 2));
	return set_sat_id(addr, 2 * (chip_unitnum % 2));
}

struct mi p9_mi = {
        .target = {
                .name = "POWER9 mi",
                .compatible = "ibm,power9-mi",
                .class = "mi",
		.translate = translate_cast(p9_mi_translate),
        },
};
DECLARE_HW_UNIT(p9_mi);

static uint64_t p9_dmi_translate(struct dmi *dmi, uint64_t addr)
{
	uint8_t ring = get_ring(addr);
	uint8_t chiplet_id = get_chiplet_id(addr);
	uint8_t sat_offset = get_sat_offset(addr);
	int chip_unitnum = pdbg_target_index(t(dmi));

	if (chiplet_id == N3_CHIPLET_ID || chiplet_id == N1_CHIPLET_ID)
	{
		//SCOM3   (See mc_clscom_rlm.fig <= 0xB vs mc_scomfir_rlm.fig > 0xB)
		//DMI0           05     02       0   0x2X (X <= 0xB)
		//DMI1           05     02       0   0x3X (X <= 0xB)
		//DMI2           05     02       2   0x2X (X <= 0xB)
		//DMI3           05     02       2   0x3X (X <= 0xB)
		//DMI4           03     02       0   0x2X (X <= 0xB)
		//DMI5           03     02       0   0x3X (X <= 0xB)
		//DMI6           03     02       2   0x2X (X <= 0xB)
		//DMI7           03     02       2   0x3X (X <= 0xB)
		addr = set_chiplet_id(addr, N3_CHIPLET_ID - 2 * (chip_unitnum / 4));
		addr = set_sat_id(addr, 2 * ((chip_unitnum / 2) % 2));
		sat_offset = (sat_offset & 0xf) + ((2 + (chip_unitnum % 2)) << 4);

		return set_sat_offset(addr, sat_offset);
	} else if (chiplet_id == MC01_CHIPLET_ID || chiplet_id == MC23_CHIPLET_ID) {
		if (ring == P9C_MC_CHAN_RING_ID)
		{
			/*
			 * -------------------------------------------
			 * DMI
			 *-------------------------------------------
			 * SCOM1,2
			 * DMI0           07     02       0
			 * DMI1           07     02       1
			 * DMI2           07     02       2
			 * DMI3           07     02       3
			 * DMI4           08     02       0
			 * DMI5           08     02       1
			 * DMI6           08     02       2
			 * DMI7           08     02       3
			 */
			uint8_t msat = get_sat_id(addr) & 0xc;

			addr = set_chiplet_id(addr, MC01_CHIPLET_ID + chip_unitnum / 4);
			return set_sat_id(addr, msat + chip_unitnum % 4);
		} else if (ring == P9C_MC_BIST_RING_ID) {
			/*
			 * SCOM4
			 * DMI0           07     08     0xD   0x0X
			 * DMI1           07     08     0xD   0x1X
			 * DMI2           07     08     0xD   0x2X
			 * DMI3           07     08     0xD   0x3X
			 * DMI4           08     08     0xD   0x0X
			 * DMI5           08     08     0xD   0x1X
			 * DMI6           08     08     0xD   0x2X
			 * DMI7           08     08     0xD   0x3X
			 */
			addr = set_chiplet_id(addr, MC01_CHIPLET_ID + chip_unitnum / 4);
			sat_offset = (sat_offset & 0xf) + ((chip_unitnum % 4) << 4);
			return set_sat_offset(addr, sat_offset);
		} else if (ring == P9C_MC_IO_RING_ID) {
			/*
			 * -------------------------------------------
			 *  DMI IO
			 * -------------------------------------------
			 *           Chiplet   Ring   Satid    Off    RXTXGrp
			 * DMI0           07     04       0   0x3F       0x00
			 * DMI1           07     04       0   0x3F       0x01
			 * DMI2           07     04       0   0x3F       0x02
			 * DMI3           07     04       0   0x3F       0x03
			 * DMI4           08     04       0   0x3F       0x00
			 * DMI5           08     04       0   0x3F       0x01
			 * DMI6           08     04       0   0x3F       0x02
			 * DMI7           08     04       0   0x3F       0x03

			 * DMI0           07     04       0   0x3F       0x20
			 * DMI1           07     04       0   0x3F       0x21
			 * DMI2           07     04       0   0x3F       0x22
			 * DMI3           07     04       0   0x3F       0x23
			 * DMI4           08     04       0   0x3F       0x20
			 * DMI5           08     04       0   0x3F       0x21
			 * DMI6           08     04       0   0x3F       0x22
			 * DMI7           08     04       0   0x3F       0x23
			 *
			 * 0 MC01.CHAN0  IOM01.TX_WRAP.TX3
			 * 1 MC01.CHAN1  IOM01.TX_WRAP.TX2
			 * 2 MC01.CHAN2  IOM01.TX_WRAP.TX0
			 * 3 MC01.CHAN3  IOM01.TX_WRAP.TX1
			 * 4 MC23.CHAN0  IOM23.TX_WRAP.TX3
			 * 5 MC23.CHAN1  IOM23.TX_WRAP.TX2
			 * 6 MC23.CHAN2  IOM23.TX_WRAP.TX0
			 * 7 MC23.CHAN3  IOM23.TX_WRAP.TX1
			 *  3, 2, 0, 1
			 */
			uint8_t rxtx_grp = get_rxtx_group_id(addr);

			addr = set_chiplet_id(addr, MC01_CHIPLET_ID + (chip_unitnum / 4));
			rxtx_grp = rxtx_grp & 0xF0;

			switch (chip_unitnum % 4)
			{
			case  0:
				rxtx_grp += 3;
				break;

			case  1:
				rxtx_grp += 2;
				break;

			case  2:
				rxtx_grp += 0;
				break;

			case  3:
				rxtx_grp += 1;
				break;
			}

			return set_rxtx_group_id(addr, rxtx_grp); // 3,2,0,1
		}
	}

	pdbg_log(PDBG_WARNING, "Invalid dmi address 0x%016" PRIx64 "\n", addr);
	return addr;
}

struct dmi p9_dmi = {
        .target = {
                .name = "POWER9 dmi",
                .compatible = "ibm,power9-dmi",
                .class = "dmi",
		.translate = translate_cast(p9_dmi_translate),
        },
};
DECLARE_HW_UNIT(p9_dmi);

struct obus p9_obus = {
        .target = {
                .name = "POWER9 obus",
                .compatible = "ibm,power9-obus",
                .class = "obus",
        },
};
DECLARE_HW_UNIT(p9_obus);

struct obus_brick p9_obus_brick = {
        .target = {
                .name = "POWER9 obus_brick",
                .compatible = "ibm,power9-obus_brick",
                .class = "obus_brick",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_obus_brick);

struct sbe p9_sbe = {
        .target = {
                .name = "POWER9 sbe",
                .compatible = "ibm,power9-sbe",
                .class = "sbe",
        },
};
DECLARE_HW_UNIT(p9_sbe);

static uint64_t p9_ppe_translate(struct ppe *ppe, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(ppe));

	/* PPE SBE */
	if (chip_unitnum == PPE_SBE_CHIPUNIT_NUM) {
		addr = set_chiplet_id(addr, PIB_CHIPLET_ID);
		addr = set_port(addr, SBE_PORT_ID);
		addr = set_ring(addr, PPE_SBE_RING_ID);
		addr = set_sat_id(addr, PPE_SBE_SAT_ID);
		addr = set_sat_offset(addr, get_sat_offset(addr) & 0xf);
		return addr;
	} else {
		/* Need to set SAT offset if address is that of PPE SBE */
		if (get_port(addr) == SBE_PORT_ID) {
                        /* Adjust offset if input address is of SBE
			 * (ex: 000E0005 --> GPE: xxxxxx1x) */
                        addr = set_sat_offset(addr, get_sat_offset(addr) | 0x10);
		}

		if (chip_unitnum >= PPE_GPE0_CHIPUNIT_NUM &&
		    chip_unitnum <= PPE_GPE3_CHIPUNIT_NUM)
		{
			/* PPE GPE */
                        addr = set_chiplet_id(addr, PIB_CHIPLET_ID);
                        addr = set_port(addr, GPE_PORT_ID);
                        addr = set_ring(addr, (chip_unitnum - PPE_GPE0_CHIPUNIT_NUM) * 8);
                        addr = set_sat_id(addr, PPE_GPE_SAT_ID);
		} else if (chip_unitnum >= PPE_EQ0_CME0_CHIPUNIT_NUM &&
			   chip_unitnum <= PPE_EQ5_CME1_CHIPUNIT_NUM) {
			/* PPE CME */
                        if (chip_unitnum >= PPE_EQ0_CME1_CHIPUNIT_NUM)
				addr = set_chiplet_id(addr, EP00_CHIPLET_ID +
						      chip_unitnum % PPE_EQ0_CME1_CHIPUNIT_NUM);
                        else
				addr = set_chiplet_id(addr, EP00_CHIPLET_ID +
						      chip_unitnum % PPE_EQ0_CME0_CHIPUNIT_NUM);

                        addr = set_port(addr, UNIT_PORT_ID);
                        addr = set_ring(addr, ((chip_unitnum / PPE_EQ0_CME1_CHIPUNIT_NUM) + 8) & 0xF);
                        addr = set_sat_id(addr, PPE_CME_SAT_ID);
		} else if (chip_unitnum >= PPE_IO_XBUS_CHIPUNIT_NUM &&
			   chip_unitnum <= PPE_IO_OB3_CHIPUNIT_NUM) {
			/* PPE IO (XBUS/OBUS) */
			addr = set_chiplet_id(addr, XB_CHIPLET_ID +
					      chip_unitnum % PPE_IO_XBUS_CHIPUNIT_NUM +
					      (chip_unitnum / PPE_IO_OB0_CHIPUNIT_NUM) * 2);
                        addr = set_port(addr, UNIT_PORT_ID);

                        if (chip_unitnum == PPE_IO_XBUS_CHIPUNIT_NUM)
				addr = set_ring(addr, XB_IOPPE_0_RING_ID & 0xF);
                        else
				addr = set_ring(addr, OB_PPE_RING_ID & 0xF);

                        addr = set_sat_id(addr,OB_PPE_SAT_ID); // Same SAT_ID value for XBUS
		} else if (chip_unitnum >= PPE_IO_DMI0_CHIPUNIT_NUM &&
			   chip_unitnum <= PPE_IO_DMI1_CHIPUNIT_NUM) {
			/* PPE IO (DMI) */
                        addr = set_chiplet_id(addr, MC01_CHIPLET_ID + (chip_unitnum - PPE_IO_DMI0_CHIPUNIT_NUM));
                        addr = set_ring(addr, MC_IOM01_1_RING_ID);
                        addr = set_port(addr, UNIT_PORT_ID);
                        addr = set_sat_id(addr, P9C_MC_PPE_SAT_ID);
		} else if (chip_unitnum >= PPE_PB0_CHIPUNIT_NUM &&
			   chip_unitnum <= PPE_PB2_CHIPUNIT_NUM) {
			/* PPE PB */
                        addr = set_chiplet_id(addr, N3_CHIPLET_ID); // TODO: Need to set ChipID for PB1 and PB2 in Cummulus
                        addr = set_port(addr, UNIT_PORT_ID);
                        addr = set_ring(addr, N3_PB_3_RING_ID & 0xF);
                        addr = set_sat_id(addr, PPE_PB_SAT_ID);
		} else {
			pdbg_log(PDBG_WARNING, "Invalid ppe address 0x%016" PRIx64 "\n", addr);
			return 0xfffffffffffffff1ull;
		}

		return addr;
	}
}

struct ppe p9_ppe = {
        .target = {
                .name = "POWER9 ppe",
                .compatible = "ibm,power9-ppe",
                .class = "ppe",
		.translate = translate_cast(p9_ppe_translate),
        },
};
DECLARE_HW_UNIT(p9_ppe);

static uint64_t p9_pec_translate(struct pec *pec, uint64_t addr)
{
	if (get_chiplet_id(addr) == N2_CHIPLET_ID)
		return set_ring(addr, (N2_PCIS0_0_RING_ID + pdbg_target_index(t(pec))) & 0xF);
	else
		return set_chiplet_id(addr, PCI0_CHIPLET_ID + pdbg_target_index(t(pec)));
}

struct pec p9_pec = {
        .target = {
                .name = "POWER9 pec",
                .compatible = "ibm,power9-pec",
                .class = "pec",
		.translate = translate_cast(p9_pec_translate),
        },
};
DECLARE_HW_UNIT(p9_pec);

static uint64_t p9_phb_translate(struct phb *phb, uint64_t addr)
{
	int chip_unitnum = pdbg_target_index(t(phb));

	if (get_chiplet_id(addr) == N2_CHIPLET_ID) {
		// nest
		if (chip_unitnum == 0)
		{
			addr = set_ring(addr, N2_PCIS0_0_RING_ID & 0xF);
			addr = set_sat_id(addr, get_sat_id(addr) < 4 ? 1 : 4);

			return addr;
		} else {
			addr = set_ring(addr, (N2_PCIS0_0_RING_ID + (chip_unitnum / 3) + 1) & 0xF);
			addr = set_sat_id(addr, (get_sat_id(addr) < 4 ? 1 : 4) +
					  (chip_unitnum % 2 ? 0 : 1) +
					  (2 * (chip_unitnum / 5)));

			return addr;
		}
	} else {
		// pci
		if (chip_unitnum == 0) {
			addr = set_chiplet_id(addr, PCI0_CHIPLET_ID);
			addr = set_sat_id(addr, (get_sat_id(addr) < 4 ? 1 : 4));

			return addr;
		} else {
			addr = set_chiplet_id(addr, PCI0_CHIPLET_ID + (chip_unitnum / 3) + 1);
			addr = set_sat_id(addr, (get_sat_id(addr) < 4 ? 1 : 4) +
					  (chip_unitnum % 2 ? 0 : 1) +
					  (2 * (chip_unitnum / 5)));

			return addr;
		}
	}
}

struct phb p9_phb = {
        .target = {
                .name = "POWER9 phb",
                .compatible = "ibm,power9-phb",
                .class = "phb",
		.translate = translate_cast(p9_phb_translate),
        },
};
DECLARE_HW_UNIT(p9_phb);

struct mc p9_mc = {
        .target = {
                .name = "POWER9 mc",
                .compatible = "ibm,power9-mc",
                .class = "mc",
        },
};
DECLARE_HW_UNIT(p9_mc);

struct mem_port p9_mem_port = {
        .target = {
                .name = "POWER9 mem_port",
                .compatible = "ibm,power9-mem_port",
                .class = "mem_port",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_mem_port);

struct nmmu p9_nmmu = {
        .target = {
                .name = "POWER9 nmmu",
                .compatible = "ibm,power9-nmmu",
                .class = "nmmu",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_nmmu);

struct pau p9_pau = {
        .target = {
                .name = "POWER9 pau",
                .compatible = "ibm,power9-pau",
                .class = "pau",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_pau);

struct iohs p9_iohs = {
        .target = {
                .name = "POWER9 iohs",
                .compatible = "ibm,power9-iohs",
                .class = "iohs",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_iohs);

struct fc p9_fc = {
        .target = {
                .name = "POWER9 fc",
                .compatible = "ibm,power9-fc",
                .class = "fc",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_fc);

struct pauc p9_pauc = {
        .target = {
                .name = "POWER9 pauc",
                .compatible = "ibm,power9-pauc",
                .class = "pauc",
		.translate = p9_unknown_translation,
        },
};
DECLARE_HW_UNIT(p9_pauc);

__attribute__((constructor))
static void register_p9_fapi_targets(void)
{
	pdbg_hwunit_register(&p9_ex_hw_unit);
	pdbg_hwunit_register(&p9_mba_hw_unit);
	pdbg_hwunit_register(&p9_mcs_hw_unit);
	pdbg_hwunit_register(&p9_xbus_hw_unit);
	pdbg_hwunit_register(&p9_abus_hw_unit);
	pdbg_hwunit_register(&p9_l4_hw_unit);
	pdbg_hwunit_register(&p9_eq_hw_unit);
	pdbg_hwunit_register(&p9_mca_hw_unit);
	pdbg_hwunit_register(&p9_mcbist_hw_unit);
	pdbg_hwunit_register(&p9_mi_hw_unit);
	pdbg_hwunit_register(&p9_dmi_hw_unit);
	pdbg_hwunit_register(&p9_obus_hw_unit);
	pdbg_hwunit_register(&p9_obus_brick_hw_unit);
	pdbg_hwunit_register(&p9_sbe_hw_unit);
	pdbg_hwunit_register(&p9_ppe_hw_unit);
	pdbg_hwunit_register(&p9_pec_hw_unit);
	pdbg_hwunit_register(&p9_phb_hw_unit);
	pdbg_hwunit_register(&p9_mc_hw_unit);
	pdbg_hwunit_register(&p9_mem_port_hw_unit);
	pdbg_hwunit_register(&p9_nmmu_hw_unit);
	pdbg_hwunit_register(&p9_pau_hw_unit);
	pdbg_hwunit_register(&p9_iohs_hw_unit);
	pdbg_hwunit_register(&p9_fc_hw_unit);
	pdbg_hwunit_register(&p9_pauc_hw_unit);
}
