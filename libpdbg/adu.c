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

#include "operations.h"
#include "bitutils.h"

/* ADU SCOM Register Definitions */
#define ALTD_CONTROL_REG	0x2020000
#define ALTD_CMD_REG		0x2020001
#define ALTD_STATUS_REG		0x2020002
#define ALTD_DATA_REG		0x2020003

/* ALTD_CMD_REG fields */
#define FBC_ALTD_START_OP	PPC_BIT(2)
#define FBC_ALTD_CLEAR_STATUS	PPC_BIT(3)
#define FBC_ALTD_RESET_AD_PCB	PPC_BIT(4)
#define FBC_ALTD_SCOPE		PPC_BITMASK(16, 18)
#define FBC_ALTD_AUTO_INC	PPC_BIT(19)
#define FBC_ALTD_DROP_PRIORITY	PPC_BITMASK(20, 21)
#define FBC_LOCKED		PPC_BIT(11)

#define DROP_PRIORITY_LOW	0ULL
#define DROP_PRIORITY_MEDIUM	1ULL
#define DROP_PRIORITY_HIGH	2ULL

#define SCOPE_NODAL		0
#define SCOPE_GROUP		1
#define SCOPE_SYSTEM		2
#define SCOPE_REMOTE		3

/* ALTD_CONTROL_REG fields */
#define FBC_ALTD_TTYPE		PPC_BITMASK(0, 5)
#define TTYPE_TREAD 		PPC_BIT(6)
#define TTYPE_TWRITE 		0
#define FBC_ALTD_TSIZE		PPC_BITMASK(7, 13)
#define FBC_ALTD_ADDRESS	PPC_BITMASK(14, 63)

#define TTYPE_CI_PARTIAL_WRITE		0b110111
#define TTYPE_CI_PARTIAL_OOO_WRITE	0b110110
#define TTYPE_DMA_PARTIAL_WRITE    	0b100110
#define TTYPE_CI_PARTIAL_READ 	 	0b110100
#define TTYPE_DMA_PARTIAL_READ 		0b110101
#define TTYPE_PBOPERATION 		0b111111

/* ALTD_STATUS_REG fields */
#define FBC_ALTD_ADDR_DONE	PPC_BIT(2)
#define FBC_ALTD_DATA_DONE	PPC_BIT(3)
#define FBC_ALTD_PBINIT_MISSING PPC_BIT(18)

static int adu_lock(struct target *target)
{
	uint64_t val;

	CHECK_ERR(read_target(target, ALTD_CMD_REG, &val));

	if (val & FBC_LOCKED)
		PR_INFO("ADU already locked! Ignoring.\n");

	val |= FBC_LOCKED;
	CHECK_ERR(write_target(target, ALTD_CMD_REG, val));

	return 0;
}

static int adu_unlock(struct target *target)
{
	uint64_t val;

	CHECK_ERR(read_target(target, ALTD_CMD_REG, &val));

	if (!(val & FBC_LOCKED)) {
		PR_INFO("ADU already unlocked!\n");
		return 0;
	}

	val &= ~FBC_LOCKED;
	CHECK_ERR(write_target(target, ALTD_CMD_REG, val));

	return 0;
}

static int adu_reset(struct target *target)
{
	uint64_t val;

	CHECK_ERR(read_target(target, ALTD_CMD_REG, &val));
	val |= FBC_ALTD_CLEAR_STATUS | FBC_ALTD_RESET_AD_PCB;
	CHECK_ERR(write_target(target, ALTD_CMD_REG, val));

	return 0;
}

/* Return size bytes of memory in *output. *output must point to an
 * array large enough to hold size bytes. */
int adu_getmem(struct target *target, uint64_t start_addr, uint8_t *output, uint64_t size)
{
	int rc = 0;
	uint64_t addr, cmd_reg, ctrl_reg, val;

	CHECK_ERR(adu_lock(target));

	ctrl_reg = TTYPE_TREAD;
	ctrl_reg = SETFIELD(FBC_ALTD_TTYPE, ctrl_reg, TTYPE_DMA_PARTIAL_READ);
	ctrl_reg = SETFIELD(FBC_ALTD_TSIZE, ctrl_reg, 8);

	CHECK_ERR(read_target(target, ALTD_CMD_REG, &cmd_reg));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

	/* We read data in 8-byte aligned chunks */
	for (addr = 8*(start_addr / 8); addr < start_addr + size; addr += 8) {
		uint64_t data;

	retry:
		/* Clear status bits */
		CHECK_ERR(adu_reset(target));

		/* Set the address */
		ctrl_reg = SETFIELD(FBC_ALTD_ADDRESS, ctrl_reg, addr);
		CHECK_ERR(write_target(target, ALTD_CONTROL_REG, ctrl_reg));

		/* Start the command */
		CHECK_ERR(write_target(target, ALTD_CMD_REG, cmd_reg));

		/* Wait for completion */
		do {
			CHECK_ERR(read_target(target, ALTD_STATUS_REG, &val));
		} while (!val);

		if( !(val & FBC_ALTD_ADDR_DONE) ||
		    !(val & FBC_ALTD_DATA_DONE)) {
			/* PBINIT_MISSING is expected occasionally so just retry */
			if (val & FBC_ALTD_PBINIT_MISSING)
				goto retry;
			else {
				PR_ERROR("Unable to read memory. "	\
					 "ALTD_STATUS_REG = 0x%016llx\n", val);
				rc = -1;
				break;
			}
		}

		/* Read data */
		CHECK_ERR(read_target(target, ALTD_DATA_REG, &data));

		/* ADU returns data in big-endian form in the register */
		data = __builtin_bswap64(data);

		if (addr < start_addr) {
			memcpy(output, ((uint8_t *) &data) + (start_addr - addr), 8 - (start_addr - addr));
			output += 8 - (start_addr - addr);
		} else if (addr + 8 > start_addr + size) {
			memcpy(output, &data, start_addr + size - addr);
		} else {
			memcpy(output, &data, 8);
			output += 8;
		}
	}

	adu_unlock(target);

	return rc;
}

int adu_putmem(struct target *target, uint64_t start_addr, uint8_t *input, uint64_t size)
{
	int rc = 0, tsize;
	uint64_t addr, cmd_reg, ctrl_reg, val, data, end_addr;

	CHECK_ERR(adu_lock(target));

	ctrl_reg = TTYPE_TWRITE;
	ctrl_reg = SETFIELD(FBC_ALTD_TTYPE, ctrl_reg, TTYPE_DMA_PARTIAL_WRITE);

	CHECK_ERR(read_target(target, ALTD_CMD_REG, &cmd_reg));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

	end_addr = start_addr + size;
	for (addr = start_addr; addr < end_addr; addr += tsize, input += tsize) {
		if ((addr % 8) || (addr + 8 > end_addr)) {
			/* If the address is not 64-bit aligned we
			 * copy in a byte at a time until it is. */
			tsize = 1;

			/* Copy the input data in with correct alignment */
			data = ((uint64_t) *input) << 8*(8 - (addr % 8) - 1);
		} else {
			tsize = 8;
			memcpy(&data, input, sizeof(data));
			data = __builtin_bswap64(data);
		}
		ctrl_reg = SETFIELD(FBC_ALTD_TSIZE, ctrl_reg, tsize);

	retry:
		/* Clear status bits */
		CHECK_ERR(adu_reset(target));

		/* Set the address */
		ctrl_reg = SETFIELD(FBC_ALTD_ADDRESS, ctrl_reg, addr);
		CHECK_ERR(write_target(target, ALTD_CONTROL_REG, ctrl_reg));

		/* Write the data */
		CHECK_ERR(write_target(target, ALTD_DATA_REG, data));

		/* Start the command */
		CHECK_ERR(write_target(target, ALTD_CMD_REG, cmd_reg));

		/* Wait for completion */
		do {
			CHECK_ERR(read_target(target, ALTD_STATUS_REG, &val));
		} while (!val);

		if( !(val & FBC_ALTD_ADDR_DONE) ||
		    !(val & FBC_ALTD_DATA_DONE)) {
			/* PBINIT_MISSING is expected occasionally so just retry */
			if (val & FBC_ALTD_PBINIT_MISSING)
				goto retry;
			else {
				PR_ERROR("Unable to write memory. "	\
					 "ALTD_STATUS_REG = 0x%016llx\n", val);
				rc = -1;
				break;
			}
		}
	}

	adu_unlock(target);

	return rc;
}
