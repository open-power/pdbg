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
#include <inttypes.h>

#include "hwunit.h"
#include "bitutils.h"
#include "operations.h"
#include "debug.h"

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

static int fsi2pib_getscom(struct pib *pib, uint64_t addr, uint64_t *value)
{
	uint32_t result;

	usleep(FSI2PIB_RELAX);

	/* Get scom works by putting the address in FSI_CMD_REG and
	 * reading the result from FST_DATA[01]_REG. */
	CHECK_ERR(fsi_write(&pib->target, FSI_CMD_REG, addr));
	CHECK_ERR(fsi_read(&pib->target, FSI_DATA0_REG, &result));
	*value = ((uint64_t) result) << 32;
	CHECK_ERR(fsi_read(&pib->target, FSI_DATA1_REG, &result));
	*value |= result;

	return 0;
}

static int fsi2pib_putscom(struct pib *pib, uint64_t addr, uint64_t value)
{
	usleep(FSI2PIB_RELAX);

	CHECK_ERR(fsi_write(&pib->target, FSI_DATA0_REG, (value >> 32) & 0xffffffff));
	CHECK_ERR(fsi_write(&pib->target, FSI_DATA1_REG, value & 0xffffffff));
	CHECK_ERR(fsi_write(&pib->target, FSI_CMD_REG, FSI_CMD_REG_WRITE | addr));

	return 0;
}

static int fsi2pib_reset(struct pdbg_target *target)
{
	/* Reset the PIB master interface. We used to reset the entire FSI2PIB
	 * engine but that had the unfortunate side effect of clearing existing
	 * settings such as the true mask register (0xd) */
	CHECK_ERR(fsi_write(target, FSI_SET_PIB_RESET_REG, FSI_SET_PIB_RESET));
	return 0;
}

static struct pib fsi_pib = {
	.target = {
		.name =	"POWER FSI2PIB",
		.compatible = "ibm,fsi-pib",
		.class = "pib",
		.probe = fsi2pib_reset,
	},
	.read = fsi2pib_getscom,
	.write = fsi2pib_putscom,
	.fd = -1,
};
DECLARE_HW_UNIT(fsi_pib);

static uint64_t opb_poll(struct opb *opb, uint32_t *read_data)
{
	unsigned long retries = MFSI_OPB_MAX_TRIES;
	uint64_t sval;
	uint32_t stat;
	int64_t rc;

	/* We try again every 10us for a bit more than 1ms */
	for (;;) {
		/* Read OPB status register */
		rc = pib_read(&opb->target, PIB2OPB_REG_STAT, &sval);
		if (rc) {
			/* Do something here ? */
			PR_ERROR("XSCOM error %" PRId64 " read OPB STAT\n", rc);
			return -1;
		}
		PR_DEBUG("  STAT=0x%16" PRIx64 "...\n", sval);

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
		pib_write(&opb->target, PIB2OPB_REG_RESET, PPC_BIT(0));
		pib_write(&opb->target, PIB2OPB_REG_STAT, PPC_BIT(0));
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

static int p8_opb_read(struct opb *opb, uint32_t addr, uint32_t *data)
{
	uint64_t opb_cmd = OPB_CMD_READ | OPB_CMD_32BIT;
	int64_t rc;

	if (addr > 0x00ffffff)
		return OPB_ERR_BAD_OPB_ADDR;

	/* Turn the address into a byte address */
       	addr = (addr & 0xffff00) | ((addr & 0xff) << 2);
	opb_cmd |= addr;
	opb_cmd <<= 32;

	PR_DEBUG("MFSI_OPB_READ: Writing 0x%16" PRIx64 "\n", opb_cmd);

	rc = pib_write(&opb->target, PIB2OPB_REG_CMD, opb_cmd);
	if (rc) {
		PR_ERROR("XSCOM error %" PRId64 " writing OPB CMD\n", rc);
		return OPB_ERR_XSCOM_ERR;
	}
	return opb_poll(opb, data);
}

static int p8_opb_write(struct opb *opb, uint32_t addr, uint32_t data)
{
	uint64_t opb_cmd = OPB_CMD_WRITE | OPB_CMD_32BIT;
	int64_t rc;

	if (addr > 0x00ffffff)
		return OPB_ERR_BAD_OPB_ADDR;

       	addr = (addr & 0xffff00) | ((addr & 0xff) << 2);
	opb_cmd |= addr;
	opb_cmd <<= 32;
	opb_cmd |= data;

	PR_DEBUG("MFSI_OPB_WRITE: Writing 0x%16" PRIx64 "\n", opb_cmd);

	rc = pib_write(&opb->target, PIB2OPB_REG_CMD, opb_cmd);
	if (rc) {
		PR_ERROR("XSCOM error %" PRId64 " writing OPB CMD\n", rc);
		return OPB_ERR_XSCOM_ERR;
	}
	return opb_poll(opb, NULL);
}

static struct opb p8_opb = {
	.target = {
		.name = "POWER8 OPB",
		.compatible = "ibm,power8-opb",
		.class = "opb",
	},
	.read = p8_opb_read,
	.write = p8_opb_write,
};
DECLARE_HW_UNIT(p8_opb);

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

static int p8_hmfsi_read(struct fsi *fsi, uint32_t addr, uint32_t *data)
{
	return opb_read(&fsi->target, addr, data);
}

static int p8_hmfsi_write(struct fsi *fsi, uint32_t addr, uint32_t data)
{
	return opb_write(&fsi->target, addr, data);
}

static int p8_hmfsi_probe(struct pdbg_target *target)
{
	struct fsi *fsi = target_to_fsi(target);
	uint32_t value;
	int rc;

	if ((rc = opb_read(&fsi->target, 0xc09, &value)))
		return rc;

	fsi->chip_type = get_chip_type(value);

	PR_DEBUG("Found chip type %x\n", fsi->chip_type);
	if (fsi->chip_type == CHIP_UNKNOWN)
		return -1;

	return 0;
}

static struct fsi p8_opb_hmfsi = {
	.target = {
		.name = "POWER8 OPB attached hMFSI",
		.compatible = "ibm,power8-opb-hmfsi",
		.class = "fsi",
		.probe = p8_hmfsi_probe,
	},
	.read = p8_hmfsi_read,
	.write = p8_hmfsi_write,
};
DECLARE_HW_UNIT(p8_opb_hmfsi);

static int cfam_hmfsi_read(struct fsi *fsi, uint32_t addr, uint32_t *data)
{
	struct pdbg_target *parent_fsi = require_target_parent("fsi", &fsi->target, false);

	addr += pdbg_target_address(&fsi->target, NULL);

	return fsi_read(parent_fsi, addr, data);
}

static int cfam_hmfsi_write(struct fsi *fsi, uint32_t addr, uint32_t data)
{
	struct pdbg_target *parent_fsi = require_target_parent("fsi", &fsi->target, false);

	addr += pdbg_target_address(&fsi->target, NULL);

	return fsi_write(parent_fsi, addr, data);
}

static int cfam_hmfsi_probe(struct pdbg_target *target)
{
	struct fsi *fsi = target_to_fsi(target);
	struct pdbg_target *fsi_parent = target->parent;
	uint32_t value, port;
	int rc;

	/* Enable the port in the upstream control register */
	assert(!(pdbg_target_u32_property(target, "port", &port)));
	fsi_read(fsi_parent, 0x3404, &value);
	value |= 1 << (31 - port);
	if ((rc = fsi_write(fsi_parent, 0x3404, value))) {
		PR_ERROR("Unable to enable HMFSI port %d\n", port);
		return rc;
	}

	if ((rc = fsi_read(&fsi->target, 0xc09, &value)))
		return rc;

	fsi->chip_type = get_chip_type(value);

	PR_DEBUG("Found chip type %x\n", fsi->chip_type);
	if (fsi->chip_type == CHIP_UNKNOWN)
		return -1;

	return 0;
}

static struct fsi cfam_hmfsi = {
	.target = {
		.name = "CFAM hMFSI Port",
		.compatible = "ibm,fsi-hmfsi",
		.class = "fsi",
		.probe = cfam_hmfsi_probe,
	},
	.read = cfam_hmfsi_read,
	.write = cfam_hmfsi_write,
};
DECLARE_HW_UNIT(cfam_hmfsi);

__attribute__((constructor))
static void register_cfam(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &fsi_pib_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p8_opb_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p8_opb_hmfsi_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &cfam_hmfsi_hw_unit);
}
