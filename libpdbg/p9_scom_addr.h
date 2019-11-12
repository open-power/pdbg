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

/* Extract pervasive chiplet ID from SCOM address */
static inline uint8_t get_chiplet_id(uint64_t addr)
{
	return ((addr >> 24) & 0x3F);
}

/* Modify SCOM address to update pervasive chiplet ID */
static inline uint64_t set_chiplet_id(uint64_t addr, uint8_t chiplet_id)
{
	addr &= 0xFFFFFFFFC0FFFFFFULL;
	addr |= ((chiplet_id & 0x3F) << 24);
	return addr;
}

/* Modify SCOM address to update ring field value */
static inline uint64_t set_ring(uint64_t addr, uint8_t ring)
{
	addr &= 0xFFFFFFFFFFFF03FFULL;
	addr |= ((ring & 0x3F) << 10);
	return addr;
}

/* Modify SCOM address to update satellite ID field */
static inline uint64_t set_sat_id(uint64_t addr, uint8_t sat_id)
{
	addr &= 0xFFFFFFFFFFFFFC3FULL;
	addr |= ((sat_id & 0xF) << 6);
	return addr;
}


#endif
