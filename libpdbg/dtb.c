/* Copyright 2019 IBM Corp.
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>

#include "libpdbg.h"
#include "target.h"

#include "fake.dt.h"

#include "p8-i2c.dt.h"
#include "p8-fsi.dt.h"
#include "p8-kernel.dt.h"
#include "p9w-fsi.dt.h"
#include "p9r-fsi.dt.h"
#include "p9z-fsi.dt.h"
#include "p9-kernel.dt.h"
#include "p8-host.dt.h"
#include "p9-host.dt.h"
#include "p8-cronus.dt.h"
#include "p9-cronus.dt.h"

#define AMI_BMC "/proc/ractrends/Helper/FwInfo"
#define OPENFSI_BMC "/sys/bus/platform/devices/gpio-fsi/fsi0/"
#define FSI_CFAM_ID "/sys/devices/platform/gpio-fsi/fsi0/slave@00:00/cfam_id"
#define XSCOM_BASE_PATH "/sys/kernel/debug/powerpc/scom"

#define CHIP_ID_P8  0xea
#define CHIP_ID_P8P 0xd3
#define CHIP_ID_P9  0xd1

static enum pdbg_backend pdbg_backend = PDBG_DEFAULT_BACKEND;
static const char *pdbg_backend_option;

/* Determines the most appropriate backend for the host system we are
 * running on. */
static enum pdbg_backend default_backend(void)
{
	int rc;

	rc = access(XSCOM_BASE_PATH, F_OK);
	if (rc == 0) /* PowerPC Host System */
		return PDBG_BACKEND_HOST;

	rc = access(AMI_BMC, F_OK);
	if (rc == 0) /* AMI BMC */
		return PDBG_BACKEND_I2C;

	if (kernel_get_fsi_path() != NULL) /* Kernel interface. OpenBMC */
		return PDBG_BACKEND_KERNEL;

	return PDBG_BACKEND_FAKE;
}

/* Try and determine what system type we are on by reading
 * /proc/cpuinfo */
static void ppc_target(struct pdbg_dtb *dtb)
{
	const char *pos = NULL;
	char line[256];
	FILE *cpuinfo;

	if (pdbg_backend_option) {
		if (!strcmp(pdbg_backend_option, "p8")) {
			dtb->system = &_binary_p8_host_dtb_o_start;
			return;
		} else if (!strcmp(pdbg_backend_option, "p9")) {
			dtb->system = &_binary_p9_host_dtb_o_start;
			return;
		}
	}

	cpuinfo = fopen("/proc/cpuinfo", "r");
	if (!cpuinfo)
		return;

	while ((pos = fgets(line, sizeof(line), cpuinfo)))
		if (strncmp(line, "cpu", 3) == 0)
			break;
	fclose(cpuinfo);

	if (!pos) {
		/* Got to EOF without a break */
		pdbg_log(PDBG_ERROR, "Unable to parse /proc/cpuinfo\n");
		return;
	}

	pos = strchr(line, ':');
	if (!pos || (*(pos + 1) == '\0')) {
		pdbg_log(PDBG_ERROR, "Unable to parse /proc/cpuinfo\n");
		return;
	}

	pos += 2;

	if (strncmp(pos, "POWER8", 6) == 0) {
		pdbg_log(PDBG_INFO, "Found a POWER8 PPC host system\n");
		dtb->system = &_binary_p8_host_dtb_o_start;
	}

	if (strncmp(pos, "POWER9", 6) == 0) {
		pdbg_log(PDBG_INFO, "Found a POWER9 PPC host system\n");
		dtb->system = &_binary_p9_host_dtb_o_start;
	}

	pdbg_log(PDBG_ERROR, "Unsupported CPU type '%s'\n", pos);
}

static void bmc_target(struct pdbg_dtb *dtb)
{
	FILE *cfam_id_file;
	uint32_t cfam_id = 0;
	uint32_t chip_id = 0;
	int rc;

	if (!pdbg_backend_option) {
		/* Try and determine the correct device type */
		char *path;

		rc = asprintf(&path, "%s/fsi0/slave@00:00/cfam_id", kernel_get_fsi_path());
		if (rc < 0) {
			pdbg_log(PDBG_ERROR, "Unable create fsi path");
			return;
		}

		cfam_id_file = fopen(path, "r");
		free(path);
		if (!cfam_id_file) {
			pdbg_log(PDBG_ERROR, "Unabled to open CFAM ID file\n");
			return;
		}

		rc = fscanf(cfam_id_file, "0x%" PRIx32, &cfam_id);
		if (rc != 1) {
			pdbg_log(PDBG_ERROR, "Unable to read CFAM ID: %s", strerror(errno));
		}
		fclose(cfam_id_file);
		chip_id = (cfam_id >> 4) & 0xff;
	} else {
		if (!strcmp(pdbg_backend_option, "p9"))
			chip_id = CHIP_ID_P9;
		else if (!strcmp(pdbg_backend_option, "p8"))
			chip_id = CHIP_ID_P8;
		else
			pdbg_log(PDBG_WARNING, "Invalid OpenBMC system type '%s' specified\n",
				 pdbg_backend_option);
	}

	switch(chip_id) {
	case CHIP_ID_P9:
		pdbg_log(PDBG_INFO, "Found a POWER9 OpenBMC based system\n");
		dtb->system = &_binary_p9_kernel_dtb_o_start;
		break;

	case CHIP_ID_P8:
	case CHIP_ID_P8P:
		pdbg_log(PDBG_INFO, "Found a POWER8/8+ OpenBMC based system\n");
		dtb->system = &_binary_p8_kernel_dtb_o_start;
		break;

	default:
		pdbg_log(PDBG_ERROR, "Unrecognised Chip ID register 0x%08" PRIx32 "\n", chip_id);
	}
}

/* Opens a dtb at the given path */
static void *mmap_dtb(char *file)
{
	int fd;
	void *dtb;
	struct stat statbuf;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		pdbg_log(PDBG_ERROR, "Unable to open dtb file '%s'\n", file);
		return NULL;
	}

	if (fstat(fd, &statbuf) == -1) {
		pdbg_log(PDBG_ERROR, "Failed to stat file '%s'\n", file);
		goto fail;
	}

	dtb = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (dtb == MAP_FAILED) {
		pdbg_log(PDBG_ERROR, "Failed top mmap file '%s'\n", file);
		goto fail;
	}

	return dtb;

fail:
	close(fd);
	return NULL;
}

bool pdbg_set_backend(enum pdbg_backend backend, const char *backend_option)
{
	if (pdbg_target_root()) {
		pdbg_log(PDBG_ERROR, "pdbg_set_backend() must be called before pdbg_targets_init()\n");
		return false;
	}

	pdbg_backend = backend;
	pdbg_backend_option = backend_option;

	return true;
}

const char *pdbg_get_backend_option(void)
{
	return pdbg_backend_option;
}

/* Determines what platform we are running on and returns a pointer to
 * the fdt that is most likely to work on the system. */
void pdbg_default_dtb(struct pdbg_dtb *dtb)
{
	char *fdt = getenv("PDBG_DTB");

	*dtb = (struct pdbg_dtb) {
		.system = NULL,
	};

	if (fdt) {
		dtb->system = mmap_dtb(fdt);
		return;
	}

	if (!pdbg_backend)
		pdbg_backend = default_backend();

	switch(pdbg_backend) {
	case PDBG_BACKEND_HOST:
		ppc_target(dtb);
		break;

	case PDBG_BACKEND_I2C:
		/* I2C is only supported on POWER8 */
		pdbg_log(PDBG_INFO, "Found a POWER8 AMI BMC based system\n");
		dtb->system = &_binary_p8_i2c_dtb_o_start;
		break;

	case PDBG_BACKEND_KERNEL:
		bmc_target(dtb);
		break;

	case PDBG_BACKEND_FSI:
		if (!pdbg_backend_option) {
			pdbg_log(PDBG_ERROR, "No device type specified\n");
			pdbg_log(PDBG_ERROR, "Use 'p8' or 'p9r/p9w/p9z'\n");
			return;
		}

		if (!strcmp(pdbg_backend_option, "p8"))
			dtb->system = &_binary_p8_fsi_dtb_o_start;
		else if (!strcmp(pdbg_backend_option, "p9w"))
			dtb->system = &_binary_p9w_fsi_dtb_o_start;
		else if (!strcmp(pdbg_backend_option, "p9r"))
			dtb->system = &_binary_p9r_fsi_dtb_o_start;
		else if (!strcmp(pdbg_backend_option, "p9z"))
			dtb->system = &_binary_p9z_fsi_dtb_o_start;
		else {
			pdbg_log(PDBG_ERROR, "Invalid device type specified\n");
			pdbg_log(PDBG_ERROR, "Use 'p8' or 'p9r/p9w/p9z'\n");
			return;
		}

		break;

	case PDBG_BACKEND_CRONUS:
		if (!pdbg_backend_option) {
			pdbg_log(PDBG_ERROR, "No device type specified\n");
			pdbg_log(PDBG_ERROR, "Use p8@<server> or p9@<server>\n");
			return;
		}

		if (!strncmp(pdbg_backend_option, "p8", 2))
			dtb->system = &_binary_p8_cronus_dtb_o_start;
		else if (!strncmp(pdbg_backend_option, "p9", 2))
			dtb->system = &_binary_p9_cronus_dtb_o_start;
		else {
			pdbg_log(PDBG_ERROR, "Invalid device type specified\n");
			pdbg_log(PDBG_ERROR, "Use p8@<server> or p9@<server>\n");
			return;
		}

		break;

	default:
		pdbg_log(PDBG_WARNING, "Unable to determine a valid default backend, using fake backend for testing purposes\n");
		/* Fall through */

	case PDBG_BACKEND_FAKE:
		dtb->system = &_binary_fake_dtb_o_start;
		break;
	}
}
