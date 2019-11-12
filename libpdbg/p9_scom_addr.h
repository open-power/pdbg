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

#ifndef LIBPDBG_SCOM_ADDR
#define LIBPDBG_SCOM_ADDR

/* Helpers and defines from the ekb. See p9_scom_addr.H */

/* P9 Chiplet ID enumeration */
enum {
	PIB_CHIPLET_ID  = 0x00,     ///< PIB chiplet
	PERV_CHIPLET_ID = 0x01,     ///< TP chiplet
	N0_CHIPLET_ID   = 0x02,     ///< Nest0 (North) chiplet
	N1_CHIPLET_ID   = 0x03,     ///< Nest1 (East) chiplet
	N2_CHIPLET_ID   = 0x04,     ///< Nest2 (South) chiplet
	N3_CHIPLET_ID   = 0x05,     ///< Nest3 (West) chiplet
	XB_CHIPLET_ID   = 0x06,     ///< XBus chiplet
	MC01_CHIPLET_ID = 0x07,     ///< MC01 (West) chiplet
	MC23_CHIPLET_ID = 0x08,     ///< MC23 (East) chiplet
	OB0_CHIPLET_ID  = 0x09,     ///< OBus0 chiplet
	OB1_CHIPLET_ID  = 0x0A,     ///< OBus1 chiplet (Cumulus only)
	OB2_CHIPLET_ID  = 0x0B,     ///< OBus2 chiplet (Cumulus only)
	OB3_CHIPLET_ID  = 0x0C,     ///< OBus3 chiplet
	PCI0_CHIPLET_ID = 0x0D,     ///< PCIe0 chiplet
	PCI1_CHIPLET_ID = 0x0E,     ///< PCIe1 chiplet
	PCI2_CHIPLET_ID = 0x0F,     ///< PCIe2 chiplet
	EP00_CHIPLET_ID = 0x10,     ///< Quad0 chiplet (EX0/1)
	EP01_CHIPLET_ID = 0x11,     ///< Quad1 chiplet (EX2/3)
	EP02_CHIPLET_ID = 0x12,     ///< Quad2 chiplet (EX4/5)
	EP03_CHIPLET_ID = 0x13,     ///< Quad3 chiplet (EX6/7)
	EP04_CHIPLET_ID = 0x14,     ///< Quad4 chiplet (EX8/9)
	EP05_CHIPLET_ID = 0x15,     ///< Quad5 chiplet (EX10/11)
	EC00_CHIPLET_ID = 0x20,     ///< Core0 chiplet (Quad0, EX0, C0)
	EC01_CHIPLET_ID = 0x21,     ///< Core1 chiplet (Quad0, EX0, C1)
	EC02_CHIPLET_ID = 0x22,     ///< Core2 chiplet (Quad0, EX1, C0)
	EC03_CHIPLET_ID = 0x23,     ///< Core3 chiplet (Quad0, EX1, C1)
	EC04_CHIPLET_ID = 0x24,     ///< Core4 chiplet (Quad1, EX2, C0)
	EC05_CHIPLET_ID = 0x25,     ///< Core5 chiplet (Quad1, EX2, C1)
	EC06_CHIPLET_ID = 0x26,     ///< Core6 chiplet (Quad1, EX3, C0)
	EC07_CHIPLET_ID = 0x27,     ///< Core7 chiplet (Quad1, EX3, C1)
	EC08_CHIPLET_ID = 0x28,     ///< Core8 chiplet (Quad2, EX4, C0)
	EC09_CHIPLET_ID = 0x29,     ///< Core9 chiplet (Quad2, EX4, C1)
	EC10_CHIPLET_ID = 0x2A,     ///< Core10 chiplet (Quad2, EX5, C0)
	EC11_CHIPLET_ID = 0x2B,     ///< Core11 chiplet (Quad2, EX5, C1)
	EC12_CHIPLET_ID = 0x2C,     ///< Core12 chiplet (Quad3, EX6, C0)
	EC13_CHIPLET_ID = 0x2D,     ///< Core13 chiplet (Quad3, EX6, C1)
	EC14_CHIPLET_ID = 0x2E,     ///< Core14 chiplet (Quad3, EX7, C0)
	EC15_CHIPLET_ID = 0x2F,     ///< Core15 chiplet (Quad3, EX7, C1)
	EC16_CHIPLET_ID = 0x30,     ///< Core16 chiplet (Quad4, EX8, C0)
	EC17_CHIPLET_ID = 0x31,     ///< Core17 chiplet (Quad4, EX8, C1)
	EC18_CHIPLET_ID = 0x32,     ///< Core18 chiplet (Quad4, EX9, C0)
	EC19_CHIPLET_ID = 0x33,     ///< Core19 chiplet (Quad4, EX9, C1)
	EC20_CHIPLET_ID = 0x34,     ///< Core20 chiplet (Quad5, EX10, C0)
	EC21_CHIPLET_ID = 0x35,     ///< Core21 chiplet (Quad5, EX10, C1)
	EC22_CHIPLET_ID = 0x36,     ///< Core22 chiplet (Quad5, EX11, C0)
	EC23_CHIPLET_ID = 0x37      ///< Core23 chiplet (Quad5, EX11, C1)
};

/* P9 N2 chiplet SCOM ring ID enumeration */
enum {
	N2_PSCM_RING_ID = 0x0,      ///< PSCOM
	N2_PERV_RING_ID = 0x1,      ///< PERV
	N2_CXA1_0_RING_ID = 0x2,    ///< CXA1_0
	N2_PCIS0_0_RING_ID = 0x3,   ///< PCIS0_0
	N2_PCIS1_0_RING_ID = 0x4,   ///< PCIS1_0
	N2_PCIS2_0_RING_ID = 0x5,   ///< PCIS2_0
	N2_IOPSI_0_RING_ID = 0x6    ///< IOPSI_0
};

enum {
        N3_PSCM_RING_ID = 0x0,      ///< PSCOM
        N3_PERV_RING_ID = 0x1,      ///< PERV
        N3_MC01_0_RING_ID = 0x2,    ///< MC01_0
        N3_NPU_0_RING_ID = 0x4,     ///< NPU_0
        N3_NPU_1_RING_ID = 0x5,     ///< NPU_1
        N3_PB_0_RING_ID = 0x6,      ///< PB_0
        N3_PB_1_RING_ID = 0x7,      ///< PB_1
        N3_PB_2_RING_ID = 0x8,      ///< PB_2
        N3_PB_3_RING_ID = 0x9,      ///< PB_3
        N3_BR_0_RING_ID = 0xa,      ///< BR_0
        N3_MM_0_RING_ID = 0xb,      ///< MM_0
        N3_INT_0_RING_ID = 0xc,     ///< INT_0
        N3_PB_4_RING_ID = 0xd,      ///< PB_4
        N3_PB_5_RING_ID = 0xe,      ///< PB_5
        N3_NPU_2_RING_ID = 0xf,     ///< NPU_2
};

enum {
        MC_PSCM_RING_ID = 0x0,      ///< PSCOM
        MC_PERV_RING_ID = 0x1,      ///< PERV
        MC_MC01_0_RING_ID = 0x2,    ///< MC01_0 / MC23_0
        MC_MCTRA_0_RING_ID = 0x3,   ///< MCTRA01_0 / MCTRA23_0
        MC_IOM01_0_RING_ID = 0x4,   ///< IOM01_0 / IOM45_0
        MC_IOM01_1_RING_ID = 0x5,   ///< IOM01_1 / IOM45_1
        MC_IOM23_0_RING_ID = 0x6,   ///< IOM23_0 / IOM67_0
        MC_IOM23_1_RING_ID = 0x7,   ///< IOM23_1 / IOM67_1
        MC_MC01_1_RING_ID = 0x8,    ///< MC01_1 / MC23_1
};

/* Cumulus mc rings */
enum {
        P9C_MC_PSCM_RING_ID  = 0x0,
        P9C_MC_PERV_RING_ID  = 0x1,
        P9C_MC_CHAN_RING_ID  = 0x2,
        P9C_MC_MCTRA_RING_ID = 0x3,
        P9C_MC_IO_RING_ID    = 0x4,
        P9C_MC_PPE_RING_ID   = 0x5,
        P9C_MC_BIST_RING_ID  = 0x8
};

enum {
        PPE_SBE_RING_ID  = 0x00,
        PPE_GPE0_RING_ID = 0x00,
        PPE_GPE1_RING_ID = 0x08,
        PPE_GPE2_RING_ID = 0x10,
        PPE_GPE3_RING_ID = 0x18,
};

enum {
        XB_PSCM_RING_ID = 0x0,      ///< PSCOM
        XB_PERV_RING_ID = 0x1,      ///< PERV
        XB_IOPPE_0_RING_ID = 0x2,   ///< IOPPE
        XB_IOX_0_RING_ID = 0x3,     ///< IOX_0
        XB_IOX_1_RING_ID = 0x4,     ///< IOX_1
        XB_IOX_2_RING_ID = 0x5,     ///< IOX_2
        XB_PBIOX_0_RING_ID = 0x6,   ///< PBIOX_0
        XB_PBIOX_1_RING_ID = 0x7,   ///< PBIOX_1
        XB_PBIOX_2_RING_ID = 0x8    ///< PBIOX_2
};

enum {
        OB_PSCM_RING_ID = 0x0,      ///< PSCOM
        OB_PERV_RING_ID = 0x1,      ///< PERV
        OB_PBIOA_0_RING_ID = 0x2,   ///< PBIOA_0
        OB_IOO_0_RING_ID = 0x3,     ///< IOO_0
        OB_PPE_RING_ID = 0x4        ///< PPE
};

/*
 * P9 PPE Chip Unit Instance Number enumeration
 * PPE name        Nimbus    Cumulus   Axone
 * SBE             0         0         0
 * GPE0..3         10..13    10..13    10..13
 * CME0            20..25    20..25    20..25
 * CME1            30..35    30..35    30..35
 * IO PPE (xbus)   40        40        40
 * IO PPE (obus)   41,44     41..44    41,44
 * IO PPE (dmi)    NA        45,46     NA
 * Powerbus PPEs   50        50        50
 * IO PPE (omi)    NA        NA        56..61
 */
enum
{
        PPE_SBE_CHIPUNIT_NUM         =  0,
        PPE_GPE0_CHIPUNIT_NUM        = 10,
        PPE_GPE3_CHIPUNIT_NUM        = 13,
        PPE_EQ0_CME0_CHIPUNIT_NUM    = 20,   // Quad0-CME0
        PPE_EQ5_CME0_CHIPUNIT_NUM    = 25,   // Quad5-CME0
        PPE_EQ0_CME1_CHIPUNIT_NUM    = 30,   // Quad0-CME1
        PPE_EQ5_CME1_CHIPUNIT_NUM    = 35,   // Quad5-CME1
        PPE_IO_XBUS_CHIPUNIT_NUM     = 40,
        PPE_IO_OB0_CHIPUNIT_NUM      = 41,
        PPE_IO_OB1_CHIPUNIT_NUM      = 42,
        PPE_IO_OB2_CHIPUNIT_NUM      = 43,
        PPE_IO_OB3_CHIPUNIT_NUM      = 44,
        PPE_IO_DMI0_CHIPUNIT_NUM     = 45,
        PPE_IO_DMI1_CHIPUNIT_NUM     = 46,
        PPE_PB0_CHIPUNIT_NUM         = 50,
        PPE_PB2_CHIPUNIT_NUM         = 52,
        PPE_OMI_CHIPUNIT_NUM         = 56,
        PPE_LAST_CHIPUNIT_NUM        = PPE_OMI_CHIPUNIT_NUM,
};

enum {
        GPREG_PORT_ID = 0x0,        ///< GP registers
        UNIT_PORT_ID = 0x1,         ///< Functional registers
        CME_PORT_ID = 0x2,          ///< CME registers
        CC_PORT_ID = 0x3,           ///< Clock control registers
        FIR_PORT_ID = 0x4,          ///< Common FIR registers
        CPM_PORT_ID = 0x5,          ///< CPM registers
        GPE_PORT_ID = 0x6,          ///< PPE GPE registers (For TP only)
        SBE_PORT_ID = 0xE,          ///< SBE PM registers (For TP only)
        PCBSLV_PORT_ID = 0xF        ///< PCB Slave registers
};

enum {
        PPE_SBE_SAT_ID = 0x0,
};

enum {
        PPE_GPE_SAT_ID = 0x0,
};

enum {
        PPE_CME_SAT_ID = 0x0,
};

enum {
        PPE_PB_SAT_ID = 0x0,
};

enum {
        P9C_MC_PPE_SAT_ID = 0x1,
};

enum {
        OB_IOO_SAT_ID = 0x0,
        OB_PPE_SAT_ID = 0x1
};

/* Extract pervasive chiplet ID from SCOM address */
static uint8_t get_chiplet_id(uint64_t addr)
{
	return ((addr >> 24) & 0x3F);
}

/* Modify SCOM address to update pervasive chiplet ID */
static uint64_t set_chiplet_id(uint64_t addr, uint8_t chiplet_id)
{
	addr &= 0xFFFFFFFFC0FFFFFFULL;
	addr |= ((chiplet_id & 0x3F) << 24);
	return addr;
}

static uint8_t get_ring(uint64_t addr)
{
	return ((addr >> 10) & 0x3f);
}

/* Modify SCOM address to update ring field value */
static uint64_t set_ring(uint64_t addr, uint8_t ring)
{
	addr &= 0xFFFFFFFFFFFF03FFULL;
	addr |= ((ring & 0x3F) << 10);
	return addr;
}

static uint8_t get_sat_id(uint64_t addr)
{
	return ((addr >> 6) & 0xF);
}

/* Modify SCOM address to update satellite ID field */
static uint64_t set_sat_id(uint64_t addr, uint8_t sat_id)
{
	addr &= 0xFFFFFFFFFFFFFC3FULL;
	addr |= ((sat_id & 0xF) << 6);
	return addr;
}

static uint8_t get_rxtx_group_id(uint64_t addr)
{
	return (addr >> 37) & 0x3F;
}

static uint64_t set_rxtx_group_id(uint64_t addr, uint8_t grp_id)
{
	addr &= 0xFFFFF81FFFFFFFFFULL;
	addr |= (grp_id & 0x3FULL) << 37;

	return addr;
}

static uint8_t get_port(uint64_t addr)
{
	return ((addr >> 16) & 0xF);
}

static uint64_t set_port(uint64_t addr, uint8_t port)
{
	addr &= 0xFFFFFFFFFFF0FFFFULL;
	addr |= ((port & 0xF) << 16);

	return addr;
}

static uint8_t get_sat_offset(uint64_t addr)
{
	return addr & 0x3f;
}

static uint64_t set_sat_offset(uint64_t addr, uint8_t sat_offset)
{
	addr &= 0xFFFFFFFFFFFFFFC0ULL;
	addr |= (sat_offset & 0x3F);
	return addr;
}

#endif
