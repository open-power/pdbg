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
#include <inttypes.h>

#include "operations.h"
#include "bitutils.h"

/* P8 ADU SCOM Register Definitions */
#define P8_ALTD_CONTROL_REG	0x0
#define P8_ALTD_CMD_REG		0x1
#define P8_ALTD_STATUS_REG	0x2
#define P8_ALTD_DATA_REG	0x3

/* P9 ADU SCOM Register Definitions */
#define P9_ALTD_CONTROL_REG	0x0
#define P9_ALTD_CMD_REG		0x1
#define P9_ALTD_STATUS_REG	0x3
#define P9_ALTD_DATA_REG	0x4

/* Common ALTD_CMD_REG fields */
#define FBC_ALTD_START_OP	PPC_BIT(2)
#define FBC_ALTD_CLEAR_STATUS	PPC_BIT(3)
#define FBC_ALTD_RESET_AD_PCB	PPC_BIT(4)
#define FBC_ALTD_SCOPE		PPC_BITMASK(16, 18)
#define FBC_ALTD_AUTO_INC	PPC_BIT(19)
#define FBC_ALTD_DROP_PRIORITY	PPC_BITMASK(20, 21)
#define FBC_LOCKED		PPC_BIT(11)

/* P9_ALTD_CMD_REG fields */
#define P9_TTYPE_TREAD 	       		PPC_BIT(5)
#define P9_TTYPE_TWRITE 		0
#define P9_FBC_ALTD_TTYPE		PPC_BITMASK(25, 31)

#define P9_TTYPE_CI_PARTIAL_WRITE	0b110111
#define P9_TTYPE_CI_PARTIAL_OOO_WRITE	0b110110
#define P9_TTYPE_CI_PARTIAL_READ 	0b110100
#define P9_TTYPE_DMA_PARTIAL_READ	0b000110
#define P9_TTYPE_DMA_PARTIAL_WRITE	0b100110
#define P9_FBC_ALTD_TSIZE		PPC_BITMASK(32, 39)
#define P9_FBC_ALTD_ADDRESS		PPC_BITMASK(8, 63)

#define DROP_PRIORITY_LOW	0ULL
#define DROP_PRIORITY_MEDIUM	1ULL
#define DROP_PRIORITY_HIGH	2ULL

#define SCOPE_NODAL		0
#define SCOPE_GROUP		1
#define SCOPE_SYSTEM		2
#define SCOPE_REMOTE		3

/* P8_ALTD_CONTROL_REG fields */
#define P8_FBC_ALTD_TTYPE	PPC_BITMASK(0, 5)
#define P8_TTYPE_TREAD		PPC_BIT(6)
#define P8_TTYPE_TWRITE		0
#define P8_FBC_ALTD_TSIZE	PPC_BITMASK(7, 13)
#define P8_FBC_ALTD_ADDRESS	PPC_BITMASK(14, 63)

#define P8_TTYPE_CI_PARTIAL_WRITE	0b110111
#define P8_TTYPE_CI_PARTIAL_OOO_WRITE	0b110110
#define P8_TTYPE_DMA_PARTIAL_WRITE    	0b100110
#define P8_TTYPE_CI_PARTIAL_READ 	0b110100
#define P8_TTYPE_DMA_PARTIAL_READ 	0b110101
#define P8_TTYPE_PBOPERATION 		0b111111

/* P8/P9_ALTD_STATUS_REG fields */
#define FBC_ALTD_ADDR_DONE	PPC_BIT(2)
#define FBC_ALTD_DATA_DONE	PPC_BIT(3)
#define FBC_ALTD_PBINIT_MISSING PPC_BIT(18)

int adu_getmem(struct pdbg_target *adu_target, uint64_t start_addr, uint8_t *output, uint64_t size, int ci)
{
	struct adu *adu;
	int rc = 0;
	uint64_t addr;

	assert(!strcmp(adu_target->class, "adu"));
	adu = target_to_adu(adu_target);

	/* We read data in 8-byte aligned chunks */
	for (addr = 8*(start_addr / 8); addr < start_addr + size; addr += 8) {
		uint64_t data;

		if (adu->getmem(adu, addr, &data, ci))
			return -1;

		pdbg_progress_tick(addr - start_addr, size);

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

	pdbg_progress_tick(size, size);

	return rc;
}

int adu_putmem(struct pdbg_target *adu_target, uint64_t start_addr, uint8_t *input, uint64_t size, int ci)
{
	struct adu *adu;
	int rc = 0, tsize;
	uint64_t addr, data, end_addr;

	assert(!strcmp(adu_target->class, "adu"));
	adu = target_to_adu(adu_target);
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

		adu->putmem(adu, addr, data, tsize, ci);
		pdbg_progress_tick(addr - start_addr, size);
	}

	pdbg_progress_tick(size, size);

	return rc;
}

static int adu_lock(struct adu *adu)
{
	uint64_t val;

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &val));

	if (val & FBC_LOCKED)
		PR_INFO("ADU already locked! Ignoring.\n");

	val |= FBC_LOCKED;
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, val));

	return 0;
}

static int adu_unlock(struct adu *adu)
{
	uint64_t val;

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &val));

	if (!(val & FBC_LOCKED)) {
		PR_INFO("ADU already unlocked!\n");
		return 0;
	}

	val &= ~FBC_LOCKED;
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, val));

	return 0;
}

static int adu_reset(struct adu *adu)
{
	uint64_t val;

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &val));
	val |= FBC_ALTD_CLEAR_STATUS | FBC_ALTD_RESET_AD_PCB;
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, val));

	return 0;
}

static int p8_adu_getmem(struct adu *adu, uint64_t addr, uint64_t *data, int ci)
{
	uint64_t ctrl_reg, cmd_reg, val;

	CHECK_ERR(adu_lock(adu));

	ctrl_reg = P8_TTYPE_TREAD;
	if (ci)
		/* Do cache inhibited access */
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_CI_PARTIAL_READ);
	else
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_DMA_PARTIAL_READ);
	ctrl_reg = SETFIELD(P8_FBC_ALTD_TSIZE, ctrl_reg, 8);

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &cmd_reg));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

retry:
	/* Clear status bits */
	CHECK_ERR(adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P8_FBC_ALTD_ADDRESS, ctrl_reg, addr);
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CONTROL_REG, ctrl_reg));

	/* Start the command */
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, cmd_reg));

	/* Wait for completion */
	do {
		CHECK_ERR(pib_read(&adu->target, P8_ALTD_STATUS_REG, &val));
	} while (!val);

	if( !(val & FBC_ALTD_ADDR_DONE) ||
	    !(val & FBC_ALTD_DATA_DONE)) {
		/* PBINIT_MISSING is expected occasionally so just retry */
		if (val & FBC_ALTD_PBINIT_MISSING)
			goto retry;
		else {
			PR_ERROR("Unable to read memory. "		\
					 "ALTD_STATUS_REG = 0x%016" PRIx64 "\n", val);
			return -1;
		}
	}

	/* Read data */
	CHECK_ERR(pib_read(&adu->target, P8_ALTD_DATA_REG, data));

	adu_unlock(adu);

	return 0;
}

int p8_adu_putmem(struct adu *adu, uint64_t addr, uint64_t data, int size, int ci)
{
	int rc = 0;
	uint64_t cmd_reg, ctrl_reg, val;
	CHECK_ERR(adu_lock(adu));

	ctrl_reg = P8_TTYPE_TWRITE;
	if (ci)
		/* Do cache inhibited access */
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_CI_PARTIAL_WRITE);
	else
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_DMA_PARTIAL_WRITE);
	ctrl_reg = SETFIELD(P8_FBC_ALTD_TSIZE, ctrl_reg, size);

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &cmd_reg));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

	/* Clear status bits */
	CHECK_ERR(adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P8_FBC_ALTD_ADDRESS, ctrl_reg, addr);

retry:
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CONTROL_REG, ctrl_reg));

	/* Write the data */
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_DATA_REG, data));

	/* Start the command */
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, cmd_reg));

	/* Wait for completion */
	do {
		CHECK_ERR(pib_read(&adu->target, P8_ALTD_STATUS_REG, &val));
	} while (!val);

	if( !(val & FBC_ALTD_ADDR_DONE) ||
	    !(val & FBC_ALTD_DATA_DONE)) {
		/* PBINIT_MISSING is expected occasionally so just retry */
		if (val & FBC_ALTD_PBINIT_MISSING)
			goto retry;
		else {
			PR_ERROR("Unable to write memory. "		\
				 "P8_ALTD_STATUS_REG = 0x%016" PRIx64 "\n", val);
			rc = -1;
		}
	}

	adu_unlock(adu);

	return rc;
}

static int p9_adu_getmem(struct adu *adu, uint64_t addr, uint64_t *data, int ci)
{
	uint64_t ctrl_reg, cmd_reg, val;

	cmd_reg = P9_TTYPE_TREAD;
	if (ci)
		/* Do cache inhibited access */
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_CI_PARTIAL_READ);
	else
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_DMA_PARTIAL_READ);

	/* For a read size is apparently always 0 */
	cmd_reg = SETFIELD(P9_FBC_ALTD_TSIZE, cmd_reg, 0);
 	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_REMOTE);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_LOW);

retry:
	/* Clear status bits */
	CHECK_ERR(adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P9_FBC_ALTD_ADDRESS, 0, addr);
	CHECK_ERR(pib_write(&adu->target, P9_ALTD_CONTROL_REG, ctrl_reg));

	/* Start the command */
	CHECK_ERR(pib_write(&adu->target, P9_ALTD_CMD_REG, cmd_reg));

	/* Wait for completion */
	do {
		CHECK_ERR(pib_read(&adu->target, P9_ALTD_STATUS_REG, &val));
	} while (!val);

	if( !(val & FBC_ALTD_ADDR_DONE) ||
	    !(val & FBC_ALTD_DATA_DONE)) {
		/* PBINIT_MISSING is expected occasionally so just retry */
		if (val & FBC_ALTD_PBINIT_MISSING)
			goto retry;
		else {
			PR_ERROR("Unable to read memory. "		\
					 "ALTD_STATUS_REG = 0x%016" PRIx64 "\n", val);
			return -1;
		}
	}

	/* Read data */
	CHECK_ERR(pib_read(&adu->target, P9_ALTD_DATA_REG, data));

	return 0;
}

static int p9_adu_putmem(struct adu *adu, uint64_t addr, uint64_t data, int size, int ci)
{
	uint64_t ctrl_reg, cmd_reg, val;

	/* Format to tsize. This is the "secondary encode" and is
	   shifted left on for writes. */
	size <<= 1;

	cmd_reg = P9_TTYPE_TWRITE;
	if (ci)
		/* Do cache inhibited access */
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_CI_PARTIAL_WRITE);
	else
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_DMA_PARTIAL_WRITE);
	cmd_reg = SETFIELD(P9_FBC_ALTD_TSIZE, cmd_reg, size);
 	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_REMOTE);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_LOW);

	/* Clear status bits */
	CHECK_ERR(adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P9_FBC_ALTD_ADDRESS, 0, addr);
retry:
	CHECK_ERR(pib_write(&adu->target, P9_ALTD_CONTROL_REG, ctrl_reg));

	/* Write the data */
	CHECK_ERR(pib_write(&adu->target, P9_ALTD_DATA_REG, data));

	/* Start the command */
	CHECK_ERR(pib_write(&adu->target, P9_ALTD_CMD_REG, cmd_reg));

	/* Wait for completion */
	do {
		CHECK_ERR(pib_read(&adu->target, P9_ALTD_STATUS_REG, &val));
	} while (!val);

	if( !(val & FBC_ALTD_ADDR_DONE) ||
	    !(val & FBC_ALTD_DATA_DONE)) {
		/* PBINIT_MISSING is expected occasionally so just retry */
		if (val & FBC_ALTD_PBINIT_MISSING)
			goto retry;
		else {
			PR_ERROR("Unable to read memory. "		\
					 "ALTD_STATUS_REG = 0x%016" PRIx64 "\n", val);
			return -1;
		}
	}

	return 0;
}

static struct adu p8_adu = {
	.target = {
		.name =	"POWER8 ADU",
		.compatible = "ibm,power8-adu",
		.class = "adu",
	},
	.getmem = p8_adu_getmem,
	.putmem = p8_adu_putmem,
};
DECLARE_HW_UNIT(p8_adu);

static struct adu p9_adu = {
	.target = {
		.name =	"POWER9 ADU",
		.compatible = "ibm,power9-adu",
		.class = "adu",
	},
	.getmem = p9_adu_getmem,
	.putmem = p9_adu_putmem,
};
DECLARE_HW_UNIT(p9_adu);
