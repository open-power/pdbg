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
#include "fake-backend.dt.h"

#include "p8-i2c.dt.h"
#include "p8-fsi.dt.h"
#include "p8-kernel.dt.h"
#include "p9w-fsi.dt.h"
#include "p9r-fsi.dt.h"
#include "p9z-fsi.dt.h"
#include "bmc-kernel.dt.h"
#include "bmc-kernel-rainiest.dt.h"
#include "bmc-kernel-rainier.dt.h"
#include "bmc-kernel-everest.dt.h"
#include "p8-host.dt.h"
#include "p9-host.dt.h"
#include "p10-host.dt.h"
#include "p8-cronus.dt.h"
#include "cronus.dt.h"
#include "bmc-sbefifo.dt.h"
#include "bmc-sbefifo-rainiest.dt.h"
#include "bmc-sbefifo-rainier.dt.h"
#include "bmc-sbefifo-everest.dt.h"
#include "p8.dt.h"
#include "p9.dt.h"
#include "p10.dt.h"

#define AMI_BMC "/proc/ractrends/Helper/FwInfo"
#define XSCOM_BASE_PATH "/sys/kernel/debug/powerpc/scom"

static enum pdbg_proc pdbg_proc = PDBG_PROC_UNKNOWN;
static enum pdbg_backend pdbg_backend = PDBG_DEFAULT_BACKEND;
static const char *pdbg_backend_option;

static const uint16_t ODYSSEY_CHIP_ID = 0x60C0;
static const uint8_t ATTR_TYPE_OCMB_CHIP = 75;
static const char* RAINIER = "rainier";
static const char* EVEREST = "everest";
static const char* FUJI = "fuji";
static const char* BLUERIDGE = "blueridge";

static struct pdbg_dtb pdbg_dtb = {
	.backend = {
		.fd = -1,
		.len = -1,
		.readonly = true,
	},
	.system = {
		.fd = -1,
		.len = -1,
		.readonly = true,
	},
};

static bool get_chipid(uint32_t *chip_id)
{
	FILE *cfam_id_file;
	char *path;
	uint32_t cfam_id = 0;
	int rc;

	/* Try and determine the correct device type */
	rc = asprintf(&path, "%s/fsi0/slave@00:00/cfam_id", kernel_get_fsi_path());
	if (rc < 0) {
		pdbg_log(PDBG_ERROR, "Unable to create fsi path");
		return false;
	}

	cfam_id_file = fopen(path, "r");
	free(path);
	if (!cfam_id_file) {
		pdbg_log(PDBG_ERROR, "Unable to open CFAM ID file\n");
		return false;
	}

	rc = fscanf(cfam_id_file, "0x%" PRIx32, &cfam_id);
	if (rc != 1) {
		pdbg_log(PDBG_ERROR, "Unable to read CFAM ID: %s", strerror(errno));
		fclose(cfam_id_file);
		return false;
	}
	fclose(cfam_id_file);

	*chip_id = (cfam_id >> 4) & 0xff;
	return true;
}

bool contains_substring_ignoring_case(const char* buffer, const char* substring) {
	size_t bufferLen = strlen(buffer);
	size_t substringLen = strlen(substring);

	//If the string that we are looking for is longer than the 
	//given string, return as not found
	if(substringLen > bufferLen)
	{
		return false;
	}

	for (size_t i = 0; i <= bufferLen - substringLen; ++i) {
		size_t j;
		for (j = 0; j < substringLen; ++j) {
			if (tolower(buffer[i + j]) != tolower(substring[j]))
				break;
		}
		if (j == substringLen)
			return true; // Substring found
	}
	return false; // Substring not found
}

static char* get_p10_system_type()
{
	FILE* file = fopen("/proc/device-tree/model", "r");
	if (file == NULL) {
		pdbg_log(PDBG_ERROR, "Unable to read device-tree model file: %s", strerror(errno));
		return "";
	}

	// Determine the size of the file
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate memory for the file content
	char* buffer = (char*)malloc(file_size + 1);

	if (buffer == NULL) {
		pdbg_log(PDBG_ERROR, "Memory allocation failed");
		return "";
	}

	// Read the file content into the buffer
	size_t bytes_read = fread(buffer, 1, file_size, file);

	if (bytes_read != file_size) {
		pdbg_log(PDBG_ERROR, "Error reading file");
		return "";
	}

	// Null-terminate the buffer to create a valid C-string
	buffer[bytes_read] = '\0';

	// Close the file
	fclose(file);

	return buffer;
}

/* Determines the most appropriate backend for the host system we are
 * running on. */
static enum pdbg_backend default_backend(void)
{
	const char *tmp;
	int rc;

	tmp = getenv("PDBG_BACKEND_DRIVER");
	if (tmp) {
		if (!strcmp(tmp, "fsi"))
			return PDBG_BACKEND_FSI;
		else if (!strcmp(tmp, "i2c"))
			return PDBG_BACKEND_I2C;
		else if (!strcmp(tmp, "kernel"))
			return PDBG_BACKEND_KERNEL;
		else if (!strcmp(tmp, "fake"))
			return PDBG_BACKEND_FAKE;
		else if (!strcmp(tmp, "host"))
			return PDBG_BACKEND_HOST;
		else if (!strcmp(tmp, "cronus"))
			return PDBG_BACKEND_CRONUS;
		else if (!strcmp(tmp, "sbefifo"))
			return PDBG_BACKEND_SBEFIFO;
	}

	rc = access(XSCOM_BASE_PATH, F_OK);
	if (rc == 0) /* PowerPC Host System */
		return PDBG_BACKEND_HOST;

	rc = access(AMI_BMC, F_OK);
	if (rc == 0) /* AMI BMC */
		return PDBG_BACKEND_I2C;

	if (kernel_get_fsi_path() != NULL) { /* Kernel interface. OpenBMC */
		uint32_t chip_id = 0;

		if (get_chipid(&chip_id)) {
			if (chip_id == CHIP_ID_P10) {
				return PDBG_BACKEND_SBEFIFO;
			}
		}
		return PDBG_BACKEND_KERNEL;
	}

	pdbg_log(PDBG_WARNING, "Can not determine an appropriate backend. Using fake backend\n");
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
			pdbg_proc = PDBG_PROC_P8;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p8_host_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p8_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p9")) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p9_host_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p10")) {
			pdbg_proc = PDBG_PROC_P10;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p10_host_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p10_dtb_o_start;
		} else {
			pdbg_log(PDBG_ERROR, "Invalid system type %s\n", pdbg_backend_option);
			pdbg_log(PDBG_ERROR, "Use 'p8' or 'p9' or 'p10'\n");
		}

 		return;
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
		pdbg_proc = PDBG_PROC_P8;
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_p8_host_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p8_dtb_o_start;
	} else if (strncmp(pos, "POWER9", 6) == 0) {
		pdbg_proc = PDBG_PROC_P9;
		pdbg_log(PDBG_INFO, "Found a POWER9 PPC host system\n");
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_p9_host_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p9_dtb_o_start;
	} else if (strncmp(pos, "POWER10", 7) == 0) {
		pdbg_proc = PDBG_PROC_P10;
		pdbg_log(PDBG_INFO, "Found a POWER10 PPC host system\n");
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_p10_host_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p10_dtb_o_start;
	} else {
		pdbg_log(PDBG_ERROR, "Unsupported CPU type '%s'\n", pos);
 	}
}

static void bmc_target(struct pdbg_dtb *dtb)
{
	uint32_t chip_id = 0;

	if (pdbg_backend_option) {
		if (!strcmp(pdbg_backend_option, "p8")) {
			pdbg_proc = PDBG_PROC_P8;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p8_kernel_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p8_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p9")) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_bmc_kernel_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p10")) {
			pdbg_proc = PDBG_PROC_P10;
			if (!dtb->backend.fdt) {
				char *system_type = get_p10_system_type();
				if (contains_substring_ignoring_case(system_type, EVEREST) || 
					contains_substring_ignoring_case(system_type, FUJI)) {
					pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel everest target");
					dtb->backend.fdt = &_binary_bmc_kernel_everest_dtb_o_start;
				} else if (contains_substring_ignoring_case(system_type, RAINIER)|| 
					contains_substring_ignoring_case(system_type, BLUERIDGE)) {
					pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel rainier target");
					dtb->backend.fdt = &_binary_bmc_kernel_rainier_dtb_o_start;
				} else {
					pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel target");
					dtb->backend.fdt = &_binary_bmc_kernel_dtb_o_start;
				}
				free(system_type);
			}
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p10_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p12")) {
			pdbg_proc = PDBG_PROC_P12;
			pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel rainiest target");
					dtb->backend.fdt = &_binary_bmc_kernel_rainiest_dtb_o_start;
		} else {
			pdbg_log(PDBG_ERROR, "Invalid system type %s\n", pdbg_backend_option);
			pdbg_log(PDBG_ERROR, "Use 'p8', 'p9' or 'p10'\n");
		}

		return;
	}

	if (!get_chipid(&chip_id))
		return;

	switch(chip_id) {
	case CHIP_ID_P10:
		pdbg_log(PDBG_INFO, "Found a POWER10 OpenBMC based system\n");
		pdbg_proc = PDBG_PROC_P10;
		if (!dtb->backend.fdt) {
			char *system_type = get_p10_system_type();
			if (contains_substring_ignoring_case(system_type, EVEREST) || 
					contains_substring_ignoring_case(system_type, FUJI)) {
				pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel everest target");
				dtb->backend.fdt = &_binary_bmc_kernel_everest_dtb_o_start;
			} else if (contains_substring_ignoring_case(system_type, RAINIER) || 
					contains_substring_ignoring_case(system_type, BLUERIDGE)) {
				pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel rainier target");
				dtb->backend.fdt = &_binary_bmc_kernel_rainier_dtb_o_start;
			} else {
				pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel target");
				dtb->backend.fdt = &_binary_bmc_kernel_dtb_o_start;
			}
			free(system_type);
		}
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p10_dtb_o_start;
		break;
	case CHIP_ID_P12:
		pdbg_log(PDBG_INFO, "Found a POWER10 OpenBMC based system\n");
		pdbg_proc = PDBG_PROC_P12;
		if (!dtb->backend.fdt) {
				pdbg_log(PDBG_INFO, "bmc_target - loading bmc kernel rainiest target");
				dtb->backend.fdt = &_binary_bmc_kernel_rainiest_dtb_o_start;
		}
		if (!dtb->system.fdt) //TODO:P12 - This may not be needed.
			dtb->system.fdt = &_binary_p10_dtb_o_start;
		break;

	case CHIP_ID_P9:
	case CHIP_ID_P9P:
		pdbg_log(PDBG_INFO, "Found a POWER9 OpenBMC based system\n");
		pdbg_proc = PDBG_PROC_P9;
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_bmc_kernel_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p9_dtb_o_start;
		break;

	case CHIP_ID_P8:
	case CHIP_ID_P8P:
		pdbg_proc = PDBG_PROC_P8;
		pdbg_log(PDBG_INFO, "Found a POWER8/8+ OpenBMC based system\n");
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_p8_kernel_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p8_dtb_o_start;
		break;

	default:
		pdbg_log(PDBG_ERROR, "Unrecognised Chip ID register 0x%08" PRIx32 "\n", chip_id);
	}
}

static void sbefifo_target(struct pdbg_dtb *dtb)
{
	uint32_t chip_id = 0;

	if (pdbg_backend_option) {
		if (!strcmp(pdbg_backend_option, "p9")) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_bmc_sbefifo_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p10")) {
			pdbg_proc = PDBG_PROC_P10;
			if (!dtb->backend.fdt) {
				char *system_type = get_p10_system_type();
				if (contains_substring_ignoring_case(system_type, EVEREST) || 
					contains_substring_ignoring_case(system_type, FUJI)) {
					pdbg_log(PDBG_INFO,
						"sbefifo_target - loading bmc sbefifo everest target");
					dtb->backend.fdt = &_binary_bmc_sbefifo_everest_dtb_o_start;
				} else if (contains_substring_ignoring_case(system_type, RAINIER) || 
					contains_substring_ignoring_case(system_type, BLUERIDGE)) {
					pdbg_log(PDBG_INFO,
						"sbefifo_target - loading bmc sbefifo rainier target");
					dtb->backend.fdt = &_binary_bmc_sbefifo_rainier_dtb_o_start;
				} else {
					pdbg_log(PDBG_INFO, "sbefifo_target - loading bmc sbefifo target");
					dtb->backend.fdt = &_binary_bmc_sbefifo_dtb_o_start;
				}
				free(system_type);
			}
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p10_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p12")) {
			pdbg_proc = PDBG_PROC_P12;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_bmc_sbefifo_rainiest_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p10_dtb_o_start; //TODO:P12 Need to remove this
		}
		else {
			pdbg_log(PDBG_ERROR, "Invalid system type %s\n", pdbg_backend_option);
			pdbg_log(PDBG_ERROR, "Use 'p9' or 'p10'\n");
		}

		return;
	}

	if (!get_chipid(&chip_id))
		return;

	switch(chip_id) {
	case CHIP_ID_P10:
		pdbg_log(PDBG_INFO, "Found a POWER10 OpenBMC based system\n");
		pdbg_proc = PDBG_PROC_P10;
		if (!dtb->backend.fdt) {
			char *system_type = get_p10_system_type();
			if (contains_substring_ignoring_case(system_type, EVEREST) || 
					contains_substring_ignoring_case(system_type, FUJI)) {
				pdbg_log(PDBG_INFO,
					"sbefifo_target - loading bmc sbefifo everest target");
				dtb->backend.fdt = &_binary_bmc_sbefifo_everest_dtb_o_start;
			} else if (contains_substring_ignoring_case(system_type, RAINIER) || 
					contains_substring_ignoring_case(system_type, BLUERIDGE)) {
				pdbg_log(PDBG_INFO,
					"sbefifo_target - loading bmc sbefifo rainier target");
				dtb->backend.fdt = &_binary_bmc_sbefifo_rainier_dtb_o_start;
			} else {
				pdbg_log(PDBG_INFO, "sbefifo_target - loading bmc sbefifo target");
				dtb->backend.fdt = &_binary_bmc_sbefifo_dtb_o_start;
			}
			free(system_type);
		}
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p10_dtb_o_start;
		break;
	case CHIP_ID_P12:
		pdbg_proc = PDBG_PROC_P12;
		pdbg_log(PDBG_INFO, "Found a POWER12 OpenBMC based system\n");
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_bmc_sbefifo_rainiest_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p10_dtb_o_start; //TODO:P12
		break;

	case CHIP_ID_P9:
	case CHIP_ID_P9P:
		pdbg_proc = PDBG_PROC_P9;
		pdbg_log(PDBG_INFO, "Found a POWER9 OpenBMC based system\n");
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_bmc_sbefifo_dtb_o_start;
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p9_dtb_o_start;
		break;

	case CHIP_ID_P8:
	case CHIP_ID_P8P:
		pdbg_proc = PDBG_PROC_P8;
		pdbg_log(PDBG_ERROR, "SBEFIFO backend not supported on POWER8/8+ OpenBMC based system\n");
		break;

	default:
		pdbg_log(PDBG_ERROR, "Unrecognised Chip ID register 0x%08" PRIx32 "\n", chip_id);
	}
}

/* Opens a dtb at the given path */
static void mmap_dtb(const char *file, bool readonly, struct pdbg_mfile *mfile)
{
	int fd;
	void *dtb;
	struct stat statbuf;

	*mfile = (struct pdbg_mfile) {
		.fd = -1,
		.len = -1,
	};

	if (readonly)
		fd = open(file, O_RDONLY);
	else
		fd = open(file, O_RDWR);
	if (fd < 0) {
		pdbg_log(PDBG_ERROR, "Unable to open dtb file '%s'\n", file);
		return;
	}

	if (fstat(fd, &statbuf) == -1) {
		pdbg_log(PDBG_ERROR, "Failed to stat file '%s'\n", file);
		goto fail;
	}

	if (readonly)
		dtb = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	else
		dtb = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (dtb == MAP_FAILED) {
		pdbg_log(PDBG_ERROR, "Failed to mmap file '%s'\n", file);
		goto fail;
	}

	*mfile = (struct pdbg_mfile) {
		.fd = fd,
		.len = statbuf.st_size,
		.fdt = dtb,
		.readonly = readonly,
	};
	return;

fail:
	close(fd);
}

bool pdbg_set_backend(enum pdbg_backend backend, const char *backend_option)
{
    if (pdbg_target_root()) 
    {
        pdbg_log(PDBG_ERROR, "pdbg_set_backend() must be called before pdbg_targets_init()"
            "or after calling pdbg_release_dt_root() if a dev tree is already set\n");	
        return false;
    }

    pdbg_backend = backend;
    pdbg_backend_option = backend_option;

    return true;
}

enum pdbg_backend pdbg_get_backend(void)
{
	return pdbg_backend;
}

const char *pdbg_get_backend_option(void)
{
	return pdbg_backend_option;
}

enum pdbg_proc pdbg_get_proc(void)
{
	return pdbg_proc;
}

static void set_pdbg_proc(void)
{
	const char *proc;

	/* Allow to set processor, when device trees are overriden */
	proc = getenv("PDBG_PROC");
	if (!proc)
		return;

	if (!strcmp(proc, "p8"))
		pdbg_proc = PDBG_PROC_P8;
	else if (!strcmp(proc, "p9"))
		pdbg_proc = PDBG_PROC_P9;
}

/* Determines what platform we are running on and returns a pointer to
 * the fdt that is most likely to work on the system. */
struct pdbg_dtb *pdbg_default_dtb(void *system_fdt)
{
	struct pdbg_dtb *dtb = &pdbg_dtb;
	const char *fdt;

	dtb->backend.fdt = NULL;
	dtb->system.fdt = system_fdt;

	if (!pdbg_backend)
		pdbg_backend = default_backend();

	fdt = getenv("PDBG_BACKEND_DTB");
	if (fdt)
		mmap_dtb(fdt, false, &dtb->backend);

	fdt = getenv("PDBG_DTB");
	if (fdt)
		mmap_dtb(fdt, false, &dtb->system);

	if (dtb->backend.fdt && dtb->system.fdt) {
		set_pdbg_proc();
		goto done;
	}

	switch(pdbg_backend) {
	case PDBG_BACKEND_HOST:
		ppc_target(dtb);
		break;

	case PDBG_BACKEND_I2C:
		/* I2C is only supported on POWER8 */
		pdbg_proc = PDBG_PROC_P8;
		if (!dtb->backend.fdt) {
			pdbg_log(PDBG_INFO, "Found a POWER8 AMI BMC based system\n");
			dtb->backend.fdt = &_binary_p8_i2c_dtb_o_start;
		}
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_p8_dtb_o_start;
		break;

	case PDBG_BACKEND_KERNEL:
		bmc_target(dtb);
		break;

	case PDBG_BACKEND_FSI:
		if (!pdbg_backend_option) {
			pdbg_log(PDBG_ERROR, "No system type specified\n");
			pdbg_log(PDBG_ERROR, "Use 'p8' or 'p9r/p9w/p9z'\n");
			return NULL;
		}

		if (!strcmp(pdbg_backend_option, "p8")) {
			pdbg_proc = PDBG_PROC_P8;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p8_fsi_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p8_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p9w")) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p9w_fsi_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p9r")) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p9r_fsi_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else if (!strcmp(pdbg_backend_option, "p9z")) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p9z_fsi_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else {
			pdbg_log(PDBG_ERROR, "Invalid system type %s\n", pdbg_backend_option);
			pdbg_log(PDBG_ERROR, "Use 'p8' or 'p9r/p9w/p9z'\n");
		}
		break;

	case PDBG_BACKEND_CRONUS:
		if (!pdbg_backend_option) {
			pdbg_log(PDBG_ERROR, "No system type specified\n");
			pdbg_log(PDBG_ERROR, "Use [p8|p9|p10]@<server>\n");
			return NULL;
		}

		if (!strncmp(pdbg_backend_option, "p8", 2)) {
			pdbg_proc = PDBG_PROC_P8;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_p8_cronus_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p8_dtb_o_start;
		} else if (!strncmp(pdbg_backend_option, "p9", 2)) {
			pdbg_proc = PDBG_PROC_P9;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_cronus_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p9_dtb_o_start;
		} else if (!strncmp(pdbg_backend_option, "p10", 3)) {
			pdbg_proc = PDBG_PROC_P10;
			if (!dtb->backend.fdt)
				dtb->backend.fdt = &_binary_cronus_dtb_o_start;
			if (!dtb->system.fdt)
				dtb->system.fdt = &_binary_p10_dtb_o_start;
		} else {
			pdbg_log(PDBG_ERROR, "Invalid system type %s\n", pdbg_backend_option);
			pdbg_log(PDBG_ERROR, "Use [p8|p9|p10]@<server>\n");
		}
		break;

	case PDBG_BACKEND_SBEFIFO:
		sbefifo_target(dtb);
		break;

	default:
		pdbg_log(PDBG_WARNING, "Unable to determine a valid default backend, using fake backend for testing purposes\n");
		/* Fall through */

	case PDBG_BACKEND_FAKE:
		if (!dtb->system.fdt)
			dtb->system.fdt = &_binary_fake_dtb_o_start;
		if (!dtb->backend.fdt)
			dtb->backend.fdt = &_binary_fake_backend_dtb_o_start;
		break;
	}

done:
	return dtb;
}

bool pdbg_fdt_is_readonly(void *fdt)
{
	if (pdbg_dtb.system.fdt == fdt)
		return pdbg_dtb.system.readonly;

	if (pdbg_dtb.backend.fdt == fdt)
		return pdbg_dtb.backend.readonly;

	return true;
}

static void close_dtb(struct pdbg_mfile *mfile)
{
	if (mfile->fd != -1 && mfile->len != -1 && mfile->fdt) {
		munmap(mfile->fdt, mfile->len);
		close(mfile->fd);
	}
}

bool is_ody_ocmb_chip(struct pdbg_target *target)
{
	uint8_t type = 0;
	pdbg_target_get_attribute(target, "ATTR_TYPE", 1, 1, &type);
	if(type != ATTR_TYPE_OCMB_CHIP) {
		return false;
	}
	uint32_t chipId = 0;
	pdbg_target_get_attribute(target, "ATTR_CHIP_ID", 4, 1, &chipId);
	if(chipId == ODYSSEY_CHIP_ID) {
		return true;
	}
	return false;
}

bool is_child_of_ody_chip(struct pdbg_target *target)
{
	struct pdbg_target *ocmb = NULL;
	assert(target);

	ocmb = pdbg_target_parent("ocmb", target);
	/*If it has a parent and the parent is of odyssey ocmb chip
	return true */
	if( (ocmb) && (is_ody_ocmb_chip(ocmb)) )
		return true;
	
	return false;
}

__attribute__((destructor))
static void pdbg_close_targets(void)
{
	close_dtb(&pdbg_dtb.backend);
	close_dtb(&pdbg_dtb.system);
}
