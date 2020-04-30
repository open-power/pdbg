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
#include "debug.h"
#include "hwunit.h"

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

/* There are more general implementations of this with a loop and more
 * performant implementations using GCC builtins which aren't
 * portable. Given we only need a limited domain this is quick, easy
 * and portable. */
uint8_t blog2(uint8_t x)
{
	switch(x) {
	case 1:
		return 0;
	case 2:
		return 1;
	case 4:
		return 2;
	case 8:
		return 3;
	case 16:
		return 4;
	case 32:
		return 5;
	case 64:
		return 6;
	default:
		assert(0);
	}
}

static int adu_read(struct mem *adu, uint64_t start_addr, uint8_t *output,
		    uint64_t size, uint8_t block_size, bool ci)
{
	uint8_t *output0;
	int rc = 0;
	uint64_t addr0, addr;

	if (!block_size)
		block_size = 8;

	output0 = output;

	/* Align start address to block_sized boundary */
	addr0 = block_size * (start_addr / block_size);

	/* We read data in block_sized aligned chunks */
	for (addr = addr0; addr < start_addr + size; addr += block_size) {
		uint64_t data;

		if (adu->getmem(adu, addr, &data, ci, block_size))
			return -1;

		/* ADU returns data in big-endian form in the register. */
		data = __builtin_bswap64(data);
		data >>= (addr & 0x7ull)*8;

		if (addr < start_addr) {
			size_t offset = start_addr - addr;
			size_t n = (size <= block_size-offset ? size : block_size-offset);

			memcpy(output, ((uint8_t *) &data) + offset, n);
			output += n;
		} else if (addr + block_size > start_addr + size) {
			uint64_t offset = start_addr + size - addr;

			memcpy(output, &data, offset);
			output += offset;
		} else {
			memcpy(output, &data, block_size);
			output += block_size;
		}

		pdbg_progress_tick(output - output0, size);
	}

	pdbg_progress_tick(size, size);

	return rc;
}

int adu_getmem(struct pdbg_target *adu_target, uint64_t start_addr,
	       uint8_t *output, uint64_t size)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_read(adu, start_addr, output, size, 8, false);
}

int adu_getmem_ci(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *output, uint64_t size)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_read(adu, start_addr, output, size, 8, true);
}

int adu_getmem_io(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *output, uint64_t size, uint8_t blocksize)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	/* There is no equivalent for cachable memory as blocksize
	 * does not apply to cachable reads */
	return adu_read(adu, start_addr, output, size, blocksize, true);
}

int __adu_getmem(struct pdbg_target *adu_target, uint64_t start_addr,
		 uint8_t *output, uint64_t size, bool ci)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_read(adu, start_addr, output, size, 8, ci);
}

static int adu_write(struct mem *adu, uint64_t start_addr, uint8_t *input,
		     uint64_t size, uint8_t block_size, bool ci)
{
	int rc = 0, tsize;
	uint64_t addr, data, end_addr;

	if (!block_size)
		block_size = 8;

	end_addr = start_addr + size;
	for (addr = start_addr; addr < end_addr; addr += tsize, input += tsize) {
		if ((addr % block_size) || (addr + block_size > end_addr)) {
			/* If the address is not aligned to block_size
			 * we copy the data in one byte at a time
			 * until it is aligned. */
			tsize = 1;

			/* Copy the input data in with correct
			 * alignment. Bytes need to aligned to the
			 * correct byte offset in the data register
			 * regardless of address. */
			data = ((uint64_t) *input) << 8*(8 - (addr % 8) - 1);
		} else {
			tsize = block_size;
			memcpy(&data, input, block_size);
			data = __builtin_bswap64(data);
			data >>= (addr & 7ull)*8;
		}

		adu->putmem(adu, addr, data, tsize, ci, block_size);
		pdbg_progress_tick(addr - start_addr, size);
	}

	pdbg_progress_tick(size, size);

	return rc;
}

int adu_putmem(struct pdbg_target *adu_target, uint64_t start_addr,
	       uint8_t *input, uint64_t size)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_write(adu, start_addr, input, size, 8, false);
}

int adu_putmem_ci(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *input, uint64_t size)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_write(adu, start_addr, input, size, 8, true);
}

int adu_putmem_io(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *input, uint64_t size, uint8_t block_size)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_write(adu, start_addr, input, size, block_size, true);
}

int __adu_putmem(struct pdbg_target *adu_target, uint64_t start_addr,
		 uint8_t *input, uint64_t size, bool ci)
{
	struct mem *adu;

	assert(!strcmp(adu_target->class, "mem"));
	adu = target_to_mem(adu_target);

	return adu_write(adu, start_addr, input, size, 8, ci);
}

static int adu_lock(struct mem *adu)
{
	uint64_t val;

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &val));

	if (val & FBC_LOCKED)
		PR_INFO("ADU already locked! Ignoring.\n");

	val |= FBC_LOCKED;
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, val));

	return 0;
}

static int adu_unlock(struct mem *adu)
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

static int adu_reset(struct mem *adu)
{
	uint64_t val;

	CHECK_ERR(pib_read(&adu->target, P8_ALTD_CMD_REG, &val));
	val |= FBC_ALTD_CLEAR_STATUS | FBC_ALTD_RESET_AD_PCB;
	CHECK_ERR(pib_write(&adu->target, P8_ALTD_CMD_REG, val));

	return 0;
}

static int p8_adu_getmem(struct mem *adu, uint64_t addr, uint64_t *data,
			 int ci, uint8_t block_size)
{
	uint64_t ctrl_reg, cmd_reg, val;
	int rc = 0;

	CHECK_ERR(adu_lock(adu));

	ctrl_reg = P8_TTYPE_TREAD;
	if (ci) {
		/* Do cache inhibited access */
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_CI_PARTIAL_READ);
		block_size = (blog2(block_size) + 1);
	} else {
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_DMA_PARTIAL_READ);
		block_size = 0;
	}
	ctrl_reg = SETFIELD(P8_FBC_ALTD_TSIZE, ctrl_reg, block_size);

	CHECK_ERR_GOTO(out, rc = pib_read(&adu->target, P8_ALTD_CMD_REG, &cmd_reg));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

retry:
	/* Clear status bits */
	CHECK_ERR_GOTO(out, rc = adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P8_FBC_ALTD_ADDRESS, ctrl_reg, addr);
	CHECK_ERR_GOTO(out, rc = pib_write(&adu->target, P8_ALTD_CONTROL_REG, ctrl_reg));

	/* Start the command */
	CHECK_ERR_GOTO(out, rc = pib_write(&adu->target, P8_ALTD_CMD_REG, cmd_reg));

	/* Wait for completion */
	do {
		CHECK_ERR_GOTO(out, rc = pib_read(&adu->target, P8_ALTD_STATUS_REG, &val));
	} while (!val);

	if( !(val & FBC_ALTD_ADDR_DONE) ||
	    !(val & FBC_ALTD_DATA_DONE)) {
		/* PBINIT_MISSING is expected occasionally so just retry */
		if (val & FBC_ALTD_PBINIT_MISSING)
			goto retry;
		else {
			PR_ERROR("Unable to read memory. "		\
					 "ALTD_STATUS_REG = 0x%016" PRIx64 "\n", val);
			adu_unlock(adu);
			return -1;
		}
	}

	/* Read data */
	CHECK_ERR_GOTO(out, rc = pib_read(&adu->target, P8_ALTD_DATA_REG, data));

out:
	adu_unlock(adu);
	return rc;

}

int p8_adu_putmem(struct mem *adu, uint64_t addr, uint64_t data, int size,
		  int ci, uint8_t block_size)
{
	int rc = 0;
	uint64_t cmd_reg, ctrl_reg, val;
	CHECK_ERR(adu_lock(adu));

	ctrl_reg = P8_TTYPE_TWRITE;
	if (ci) {
		/* Do cache inhibited access */
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_CI_PARTIAL_WRITE);
		block_size = (blog2(block_size) + 1);
	} else {
		ctrl_reg = SETFIELD(P8_FBC_ALTD_TTYPE, ctrl_reg, P8_TTYPE_DMA_PARTIAL_WRITE);
	}
	ctrl_reg = SETFIELD(P8_FBC_ALTD_TSIZE, ctrl_reg, block_size);

	CHECK_ERR_GOTO(out, rc = pib_read(&adu->target, P8_ALTD_CMD_REG, &cmd_reg));
	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_SYSTEM);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_MEDIUM);

	/* Clear status bits */
	CHECK_ERR_GOTO(out, rc = adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P8_FBC_ALTD_ADDRESS, ctrl_reg, addr);

retry:
	CHECK_ERR_GOTO(out, rc = pib_write(&adu->target, P8_ALTD_CONTROL_REG, ctrl_reg));

	/* Write the data */
	CHECK_ERR_GOTO(out, rc = pib_write(&adu->target, P8_ALTD_DATA_REG, data));

	/* Start the command */
	CHECK_ERR_GOTO(out, rc = pib_write(&adu->target, P8_ALTD_CMD_REG, cmd_reg));

	/* Wait for completion */
	do {
		CHECK_ERR_GOTO(out, rc = pib_read(&adu->target, P8_ALTD_STATUS_REG, &val));
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

out:
	adu_unlock(adu);

	return rc;
}

static int p9_adu_getmem(struct mem *adu, uint64_t addr, uint64_t *data,
			 int ci, uint8_t block_size)
{
	uint64_t ctrl_reg, cmd_reg, val;

	cmd_reg = P9_TTYPE_TREAD;
	if (ci) {
		/* Do cache inhibited access */
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_CI_PARTIAL_READ);
		block_size = (blog2(block_size) + 1) << 1;
	} else {
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_DMA_PARTIAL_READ);

		/* For normal reads the size is ignored as HW always
		 * returns a cache line */
		block_size = 0;
	}

	cmd_reg = SETFIELD(P9_FBC_ALTD_TSIZE, cmd_reg, block_size);
 	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_REMOTE);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_LOW);

retry:
	/* Clear status bits */
	CHECK_ERR(adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P9_FBC_ALTD_ADDRESS, 0ULL, addr);
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

static int p9_adu_putmem(struct mem *adu, uint64_t addr, uint64_t data, int size,
			 int ci, uint8_t block_size)
{
	uint64_t ctrl_reg, cmd_reg, val;

	/* Format to tsize. This is the "secondary encode" and is
	   shifted left on for writes. */
	size <<= 1;

	cmd_reg = P9_TTYPE_TWRITE;
	if (ci) {
		/* Do cache inhibited access */
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_CI_PARTIAL_WRITE);
		block_size = (blog2(block_size) + 1) << 1;
	} else {
		cmd_reg = SETFIELD(P9_FBC_ALTD_TTYPE, cmd_reg, P9_TTYPE_DMA_PARTIAL_WRITE);
		block_size <<= 1;
	}
	cmd_reg = SETFIELD(P9_FBC_ALTD_TSIZE, cmd_reg, block_size);
 	cmd_reg |= FBC_ALTD_START_OP;
	cmd_reg = SETFIELD(FBC_ALTD_SCOPE, cmd_reg, SCOPE_REMOTE);
	cmd_reg = SETFIELD(FBC_ALTD_DROP_PRIORITY, cmd_reg, DROP_PRIORITY_LOW);

	/* Clear status bits */
	CHECK_ERR(adu_reset(adu));

	/* Set the address */
	ctrl_reg = SETFIELD(P9_FBC_ALTD_ADDRESS, 0ULL, addr);
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

static struct mem p8_adu = {
	.target = {
		.name =	"POWER8 ADU",
		.compatible = "ibm,power8-adu",
		.class = "mem",
	},
	.getmem = p8_adu_getmem,
	.putmem = p8_adu_putmem,
	.read = adu_read,
	.write = adu_write,
};
DECLARE_HW_UNIT(p8_adu);

static struct mem p9_adu = {
	.target = {
		.name =	"POWER9 ADU",
		.compatible = "ibm,power9-adu",
		.class = "mem",
	},
	.getmem = p9_adu_getmem,
	.putmem = p9_adu_putmem,
	.read = adu_read,
	.write = adu_write,
};
DECLARE_HW_UNIT(p9_adu);

__attribute__((constructor))
static void register_adu(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p8_adu_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p9_adu_hw_unit);
}
