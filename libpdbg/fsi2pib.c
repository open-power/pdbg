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
#include <unistd.h>

#include "target.h"
#include "bitutils.h"
#include "operations.h"

#define FSI_DATA0_REG	0x1000
#define FSI_DATA1_REG	0x1001
#define FSI_CMD_REG	0x1002
#define  FSI_CMD_REG_WRITE PPC_BIT32(0)

#define FSI_RESET_REG	0x1006
#define  FSI_RESET_CMD PPC_BIT32(0)

#define FSI_SET_PIB_RESET_REG 0x1007
#define  FSI_SET_PIB_RESET PPC_BIT32(0)

/* For some reason the FSI2PIB engine dies with frequent
 * access. Letting it have a bit of a rest seems to stop the
 * problem. This sets the number of usecs to sleep between SCOM
 * accesses. */
#define FSI2PIB_RELAX	50

static int fsi2pib_getscom(struct pdbg_target *target, uint64_t addr, uint64_t *value)
{
	uint64_t result;

	usleep(FSI2PIB_RELAX);

	/* Get scom works by putting the address in FSI_CMD_REG and
	 * reading the result from FST_DATA[01]_REG. */
	CHECK_ERR(write_next_target(target, FSI_CMD_REG, addr));
	CHECK_ERR(read_next_target(target, FSI_DATA0_REG, &result));
	*value = result << 32;
	CHECK_ERR(read_next_target(target, FSI_DATA1_REG, &result));
	*value |= result;
	return 0;
}

static int fsi2pib_putscom(struct pdbg_target *target, uint64_t addr, uint64_t value)
{
	usleep(FSI2PIB_RELAX);

	CHECK_ERR(write_next_target(target, FSI_RESET_REG, FSI_RESET_CMD));
	CHECK_ERR(write_next_target(target, FSI_DATA0_REG, (value >> 32) & 0xffffffff));
	CHECK_ERR(write_next_target(target, FSI_DATA1_REG, value & 0xffffffff));
	CHECK_ERR(write_next_target(target, FSI_CMD_REG, FSI_CMD_REG_WRITE | addr));

	return 0;
}

int fsi2pib_target_init(struct pdbg_target *target, const char *name, uint64_t base, struct pdbg_target *next)
{
	target->name = name;
	target->read = fsi2pib_getscom;
	target->write = fsi2pib_putscom;
	target->base = base;
	target->next = next;

	return -1;
}
