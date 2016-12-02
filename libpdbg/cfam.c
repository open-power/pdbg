/* Copyright 2016 IBM Corp.
 *
 * Most of the PIB2OPB code is based on code from skiboot.
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
#include <stdio.h>

#include "target.h"
#include "bitutils.h"
#include "operations.h"

#define FSI_DATA0_REG	0x0
#define FSI_DATA1_REG	0x1
#define FSI_CMD_REG	0x2
#define  FSI_CMD_REG_WRITE PPC_BIT32(0)

#define FSI_RESET_REG	0x6
#define  FSI_RESET_CMD PPC_BIT32(0)

#define FSI_SET_PIB_RESET_REG 0x7
#define  FSI_SET_PIB_RESET PPC_BIT32(0)

/* For some reason the FSI2PIB engine dies with frequent
 * access. Letting it have a bit of a rest seems to stop the
 * problem. This sets the number of usecs to sleep between SCOM
 * accesses. */
#define FSI2PIB_RELAX	50

/*
 * Bridge registers on XSCOM that allow generatoin
 * of OPB cycles
 */
#define PIB2OPB_REG_CMD		0x0
#define   OPB_CMD_WRITE		0x80000000
#define   OPB_CMD_READ		0x00000000
#define   OPB_CMD_8BIT		0x00000000
#define   OPB_CMD_16BIT		0x20000000
#define   OPB_CMD_32BIT		0x60000000
#define PIB2OPB_REG_STAT	0x1
#define   OPB_STAT_ANY_ERR	0x80000000
#define   OPB_STAT_ERR_OPB      0x7FEC0000
#define   OPB_STAT_ERRACK       0x00100000
#define   OPB_STAT_BUSY		0x00010000
#define   OPB_STAT_READ_VALID   0x00020000
#define   OPB_STAT_ERR_CMFSI    0x0000FC00
#define   OPB_STAT_ERR_HMFSI    0x000000FC
#define   OPB_STAT_ERR_BASE	(OPB_STAT_ANY_ERR | \
				 OPB_STAT_ERR_OPB | \
				 OPB_STAT_ERRACK)
#define PIB2OPB_REG_LSTAT	0x2
#define PIB2OPB_REG_RESET	0x4
#define PIB2OPB_REG_cRSIC	0x5
#define PIB2OPB_REG_cRSIM       0x6
#define PIB2OPB_REG_cRSIS	0x7
#define PIB2OPB_REG_hRSIC	0x8
#define PIB2OPB_REG_hRSIM	0x9
#define PIB2OPB_REG_hRSIS	0xA

#define OPB_ERR_XSCOM_ERR	-1;
#define OPB_ERR_TIMEOUT_ERR	-1;
#define OPB_ERR_BAD_OPB_ADDR	-1;

/* We try up to 1.2ms for an OPB access */
#define MFSI_OPB_MAX_TRIES	1200

static int fsi2pib_getscom(struct target *target, uint64_t addr, uint64_t *value)
{
	uint64_t result;

	usleep(FSI2PIB_RELAX);

	/* Get scom works by putting the address in FSI_CMD_REG and
	 * reading the result from FST_DATA[01]_REG. */
	CHECK_ERR(write_next_target(target, FSI_RESET_REG, FSI_RESET_CMD));
	CHECK_ERR(write_next_target(target, FSI_CMD_REG, addr));
	CHECK_ERR(read_next_target(target, FSI_DATA0_REG, &result));
	*value = result << 32;
	CHECK_ERR(read_next_target(target, FSI_DATA1_REG, &result));
	*value |= result;

	return 0;
}

static int fsi2pib_putscom(struct target *target, uint64_t addr, uint64_t value)
{
	usleep(FSI2PIB_RELAX);

	CHECK_ERR(write_next_target(target, FSI_RESET_REG, FSI_RESET_CMD));
	CHECK_ERR(write_next_target(target, FSI_DATA0_REG, (value >> 32) & 0xffffffff));
	CHECK_ERR(write_next_target(target, FSI_DATA1_REG, value & 0xffffffff));
	CHECK_ERR(write_next_target(target, FSI_CMD_REG, FSI_CMD_REG_WRITE | addr));

	return 0;
}

int fsi2pib_target_init(struct target *target, const char *name, uint64_t base, struct target *next)
{
	target_init(target, name, base, fsi2pib_getscom, fsi2pib_putscom, NULL, next);

	return 0;
}

static uint64_t opb_poll(struct target *target, uint64_t *read_data)
{
	unsigned long retries = MFSI_OPB_MAX_TRIES;
	uint64_t sval;
	uint32_t stat;
	int64_t rc;

	/* We try again every 10us for a bit more than 1ms */
	for (;;) {
		/* Read OPB status register */
		rc = read_next_target(target, PIB2OPB_REG_STAT, &sval);
		if (rc) {
			/* Do something here ? */
			PR_ERROR("XSCOM error %lld read OPB STAT\n", rc);
			return -1;
		}
		PR_DEBUG("  STAT=0x%16llx...\n", sval);

		stat = sval >> 32;

		/* Complete */
		if (!(stat & OPB_STAT_BUSY))
			break;
		if (retries-- == 0) {
			/* This isn't supposed to happen (HW timeout) */
			PR_ERROR("OPB POLL timeout !\n");
			return -1;
		}
		usleep(1);
	}

	/*
	 * TODO: Add the full error analysis that skiboot has. For now
	 * we just reset things so we can continue. Also need to
	 * improve error handling as we expect these occasionally when
	 * probing the system.
	 */
	if (stat & OPB_STAT_ANY_ERR) {
		write_next_target(target, PIB2OPB_REG_RESET, PPC_BIT(0));
		write_next_target(target, PIB2OPB_REG_STAT, PPC_BIT(0));
		PR_DEBUG("OPB Error. Status 0x%08x\n", stat);
		rc = -1;
	} else if (read_data) {
		if (!(stat & OPB_STAT_READ_VALID)) {
			PR_DEBUG("Read successful but no data !\n");
			rc = -1;
		}
		*read_data = sval & 0xffffffff;
	}

	return rc;
}

static int opb_read(struct target *target, uint64_t addr, uint64_t *data)
{
	uint64_t opb_cmd = OPB_CMD_READ | OPB_CMD_32BIT;
	int64_t rc;

	if (addr > 0x00ffffff)
		return OPB_ERR_BAD_OPB_ADDR;

	/* Turn the address into a byte address */
       	addr = (addr & 0xffff00) | ((addr & 0xff) << 2);
	opb_cmd |= addr;
	opb_cmd <<= 32;

	PR_DEBUG("MFSI_OPB_READ: Writing 0x%16llx to XSCOM %llx\n",
		 opb_cmd, target->base);

	rc = write_next_target(target, PIB2OPB_REG_CMD, opb_cmd);
	if (rc) {
		PR_ERROR("XSCOM error %lld writing OPB CMD\n", rc);
		return OPB_ERR_XSCOM_ERR;
	}
	return opb_poll(target, data);
}

static int opb_write(struct target *target, uint64_t addr, uint64_t data)
{
	uint64_t opb_cmd = OPB_CMD_WRITE | OPB_CMD_32BIT;
	int64_t rc;

	if (addr > 0x00ffffff)
		return OPB_ERR_BAD_OPB_ADDR;

       	addr = (addr & 0xffff00) | ((addr & 0xff) << 2);
	opb_cmd |= addr;
	opb_cmd <<= 32;
	opb_cmd |= data;

	PR_DEBUG("MFSI_OPB_WRITE: Writing 0x%16llx to XSCOM %llx\n",
		 opb_cmd, target->base);

	rc = write_next_target(target, PIB2OPB_REG_CMD, opb_cmd);
	if (rc) {
		PR_ERROR("XSCOM error %lld writing OPB CMD\n", rc);
		return OPB_ERR_XSCOM_ERR;
	}
	return opb_poll(target, NULL);
}

int opb_target_init(struct target *target, const char *name, uint64_t base, struct target *next)
{
	target_init(target, name, base, opb_read, opb_write, NULL, next);

	/* Clear any outstanding errors */
	write_next_target(target, PIB2OPB_REG_RESET, PPC_BIT(0));
	write_next_target(target, PIB2OPB_REG_STAT, PPC_BIT(0));

	return 0;
}

enum chip_type get_chip_type(uint64_t chip_id)
{
	switch(GETFIELD(PPC_BITMASK32(12, 19), chip_id)) {
	case 0xea:
		return CHIP_P8;
	case 0xd3:
		return CHIP_P8NV;
	case 0xd1:
		return CHIP_P9;
	default:
		return CHIP_UNKNOWN;
	}
}

int mfsi_target_init(struct target *target, const char *name, uint64_t base, struct target *next)
{
	target_init(target, name, base, NULL, NULL, NULL, next);

	return 0;
}

#define HMFSI_STRIDE 0x80000
int hmfsi_target_probe(struct target *cfam, struct target *targets, int max_target_count)
{
	struct target *target = targets;
	uint64_t value;
	int target_count = 0, i;

	for (i = 0; i < 8 && i < max_target_count; i++) {
		mfsi_target_init(target, "MFSI Port", 0x80000 + i * HMFSI_STRIDE, cfam);
		if (read_target(target, 0xc09, &value)) {
			target_del(target);
			continue;
		}

		target->chip_type = get_chip_type(value);
		if (target->chip_type == CHIP_UNKNOWN) {
			target_del(target);
			continue;
		}

		target++;
		target_count++;
	}

	return target_count;
}
