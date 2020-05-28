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

/* Helpers and defines from the ekb. See p10_scom_addr.H */

/* P10 Chiplet ID enumeration */
enum {
        PIB_CHIPLET_ID   = 0x00,    ///< PIB chiplet (FSI)
        PERV_CHIPLET_ID  = 0x01,    ///< TP chiplet

        N0_CHIPLET_ID    = 0x02,    ///< Nest0 (North) chiplet
        N1_CHIPLET_ID    = 0x03,    ///< Nest1 (South) chiplet

        PCI0_CHIPLET_ID  = 0x08,    ///< PCIe0 chiplet
        PCI1_CHIPLET_ID  = 0x09,    ///< PCIe1 chiplet

        MC0_CHIPLET_ID   = 0x0C,    ///< MC0 chiplet
        MC1_CHIPLET_ID   = 0x0D,    ///< MC1 chiplet
        MC2_CHIPLET_ID   = 0x0E,    ///< MC2 chiplet
        MC3_CHIPLET_ID   = 0x0F,    ///< MC3 chiplet

        PAU0_CHIPLET_ID  = 0x10,    ///< PAU0 chiplet
        PAU1_CHIPLET_ID  = 0x11,    ///< PAU1 chiplet
        PAU2_CHIPLET_ID  = 0x12,    ///< PAU2 chiplet
        PAU3_CHIPLET_ID  = 0x13,    ///< PAU3 chiplet

        AXON0_CHIPLET_ID = 0x18,    ///< AXON0 chiplet (high speed io)
        AXON1_CHIPLET_ID = 0x19,    ///< AXON1 chiplet (high speed io)
        AXON2_CHIPLET_ID = 0x1A,    ///< AXON2 chiplet (high speed io)
        AXON3_CHIPLET_ID = 0x1B,    ///< AXON3 chiplet (high speed io)
        AXON4_CHIPLET_ID = 0x1C,    ///< AXON4 chiplet (high speed io)
        AXON5_CHIPLET_ID = 0x1D,    ///< AXON5 chiplet (high speed io)
        AXON6_CHIPLET_ID = 0x1E,    ///< AXON6 chiplet (high speed io)
        AXON7_CHIPLET_ID = 0x1F,    ///< AXON7 chiplet (high speed io)

        EQ0_CHIPLET_ID   = 0x20,    ///< Quad0 chiplet (super chiplet)
        EQ1_CHIPLET_ID   = 0x21,    ///< Quad1 chiplet (super chiplet)
        EQ2_CHIPLET_ID   = 0x22,    ///< Quad2 chiplet (super chiplet)
        EQ3_CHIPLET_ID   = 0x23,    ///< Quad3 chiplet (super chiplet)
        EQ4_CHIPLET_ID   = 0x24,    ///< Quad4 chiplet (super chiplet)
        EQ5_CHIPLET_ID   = 0x25,    ///< Quad5 chiplet (super chiplet)
        EQ6_CHIPLET_ID   = 0x26,    ///< Quad6 chiplet (super chiplet)
        EQ7_CHIPLET_ID   = 0x27,    ///< Quad7 chiplet (super chiplet)
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

static uint8_t get_ring_id(uint64_t addr)
{
    return (addr >> 10) & 0xF;
}

static uint64_t set_ring_id(uint64_t addr, uint64_t ring)
{
    addr &= 0xFFFFFFFFFFFF03FFULL;
    addr |= ((ring & 0x3F) << 10);
    return addr;
}

static uint32_t get_io_lane(uint64_t addr)
{
    return (addr >> 32) & 0x1F;
}

static uint64_t set_io_lane(uint64_t addr, uint64_t lane)
{
    addr &= 0xFFFFFFE0FFFFFFFFULL;
    addr |= (lane & 0x1F) <<  32;
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

static uint8_t get_sat_offset(uint64_t addr)
{
	return addr & 0x3F;
}

static uint64_t set_sat_offset(uint64_t addr, uint8_t sat_offset)
{
	addr &= 0xFFFFFFFFFFFFFFC0ULL;
	addr |= (sat_offset & 0x3F);
	return addr;
}

static uint64_t set_io_group_addr(uint64_t addr, uint64_t group_addr)
{
    addr &= 0xFFFFFC1FFFFFFFFFULL;
    addr |= (group_addr & 0x1F) << 37;

    return addr;
}

#endif
