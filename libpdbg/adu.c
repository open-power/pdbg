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

static int adu_lock(void)
{
	uint64_t val;

	CHECK_ERR(getscom(&val, ALTD_CMD_REG));

	if (val & FBC_LOCKED)
		PR_INFO("ADU already locked! Ignoring.\n");

	val |= FBC_LOCKED;
	CHECK_ERR(putscom(val, ALTD_CMD_REG));

	return 0;
}

static int adu_unlock(void)
{
	uint64_t val;

	CHECK_ERR(getscom(&val, ALTD_CMD_REG));

	if (!(val & FBC_LOCKED)) {
		PR_INFO("ADU already unlocked!\n");
		return 0;
	}

	val &= ~FBC_LOCKED;
	CHECK_ERR(putscom(val, ALTD_CMD_REG));

	return 0;
}

static int adu_reset(void)
{
	uint64_t val;

	CHECK_ERR(getscom(&val, ALTD_CMD_REG));
	val |= FBC_ALTD_CLEAR_STATUS | FBC_ALTD_RESET_AD_PCB;
	CHECK_ERR(putscom(val, ALTD_CMD_REG));

	return 0;
}

/* Return size bytes of memory in *output. *output must point to an
 * array large enough to hold size bytes. block_size specifies whether
 * to read 1, 2, 4, or 8 bytes at a time. A value of 0 selects the
 * best size to use based on the number of bytes to read. addr must be
 * block_size aligned. */
int adu_getmem(uint64_t addr, uint8_t *output, uint64_t size, int block_size)
{
	int rc = 0;
	uint64_t i, cmd_reg, ctrl_reg, val;

	CHECK_ERR(adu_lock());

	if (!block_size) {
		/* TODO: We could optimise this better, but this will
		 * do the moment. */
		switch(size % 8) {
		case 0:
			block_size = 8;
			break;

		case 1:
		case 3:
		case 5:
		case 7:
			block_size = 1;
			break;

		case 2:
			block_size = 2;
			break;

		case 4:
			block_size = 4;
			break;
		}

		/* Fall back to byte at a time if the selected block
		 * size is not aligned to the address. */
		if (addr % block_size)
			block_size = 1;
	}

	if (addr % block_size) {
		PR_INFO("Unaligned address\n");
		return -1;
	}

	if (size % block_size) {
		PR_INFO("Invalid size\n");
		return -1;
	}

	ctrl_reg = TTYPE_TREAD;
	ctrl_reg = SETFIELD(FBC_ALTD_TTYPE, ctrl_reg, TTYPE_DMA_PARTIAL_READ);
	ctrl_reg = SETFIELD(FBC_ALTD_TSIZE, ctrl_reg, block_size);

	CHECK_ERR(getscom(&cmd_reg, ALTD_CMD_REG));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

	for (i = addr; i < addr + size; i += block_size) {
		uint64_t data;

	retry:
		/* Clear status bits */
		CHECK_ERR(adu_reset());

		/* Set the address */
		ctrl_reg = SETFIELD(FBC_ALTD_ADDRESS, ctrl_reg, i);
		CHECK_ERR(putscom(ctrl_reg, ALTD_CONTROL_REG));

		/* Start the command */
		CHECK_ERR(putscom(cmd_reg, ALTD_CMD_REG));

		/* Wait for completion */
		do {
			CHECK_ERR(getscom(&val, ALTD_STATUS_REG));
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
		CHECK_ERR(getscom(&data, ALTD_DATA_REG));

		/* ADU returns data in big-endian form in the register */
		data = __builtin_bswap64(data);

		/* TODO: Not sure where the data for a read < 8 bytes
		 * ends up */
		memcpy(output, &data, block_size);
		output += block_size;
	}

	adu_unlock();

	return rc;
}
