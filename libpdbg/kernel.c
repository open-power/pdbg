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
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <endian.h>

#include "bitutils.h"
#include "operations.h"
#include "hwunit.h"

#define OPENFSI_LEGACY_PATH "/sys/bus/platform/devices/gpio-fsi/"
#define OPENFSI_PATH "/sys/class/fsi-master/"

const char *fsi_base;

const char *kernel_get_fsi_path(void)
{
	int rc;

	if (fsi_base)
		return fsi_base;

	rc = access(OPENFSI_PATH, F_OK);
	if (rc == 0) {
		fsi_base = OPENFSI_PATH;
		return fsi_base;
	}

	rc = access(OPENFSI_LEGACY_PATH, F_OK);
	if (rc == 0) {
		fsi_base = OPENFSI_LEGACY_PATH;
		return fsi_base;
	}

	/* This is an error, but callers use this function when probing */
	PR_DEBUG("Failed to find kernel FSI path\n");

	return NULL;
}

static int kernel_fsi_getcfam(struct fsi *fsi, uint32_t addr64, uint32_t *value)
{
	printf("kernel_fsi_getcfam address 0%x \n", addr64); 
	int rc;
	uint32_t tmp, addr = (addr64 & 0x7ffc00) | ((addr64 & 0x3ff) << 2);

	rc = lseek(fsi->fd, addr, SEEK_SET);
	if (rc < 0) {
		rc = errno;
		PR_WARNING("seek failed: %s\n", strerror(errno));
		return rc;
	}

	rc = read(fsi->fd, &tmp, 4);
	if (rc < 0) {
		rc = errno;
		if ((addr64 & 0xfff) != 0xc09)
			/* We expect reads of 0xc09 to occasionally
			 * fail as the probing code uses it to see
			 * if anything is present on the link. */
			PR_ERROR("Failed to read from 0x%08" PRIx32 " (%016" PRIx32 ")\n", (uint32_t) addr, addr64);
		return rc;
	}
	*value = be32toh(tmp);
	printf("kernel_fsi_getcfam value 0%x \n", *value); 

	return 0;
}

static int kernel_fsi_putcfam(struct fsi *fsi, uint32_t addr64, uint32_t data)
{
	int rc;
	uint32_t tmp, addr = (addr64 & 0x7ffc00) | ((addr64 & 0x3ff) << 2);

	rc = lseek(fsi->fd, addr, SEEK_SET);
	if (rc < 0) {
		rc = errno;
		PR_WARNING("seek failed: %s\n", strerror(errno));
		return rc;
	}

	tmp = htobe32(data);
	rc = write(fsi->fd, &tmp, 4);
	if (rc < 0) {
		rc = errno;
		PR_ERROR("Failed to write to 0x%08" PRIx32 " (%016" PRIx32 ")\n", addr, addr64);
		return rc;
	}

	return 0;
}

static void kernel_fsi_scan_devices(void)
{
	const char one = '1';
	const char *kernel_path = kernel_get_fsi_path();
	char *path;
	int rc, fd;

	if (!kernel_path)
		return;

	rc = asprintf(&path, "%s/fsi0/rescan", kernel_path);
	if (rc < 0) {
		PR_ERROR("Unable to create fsi path\n");
		return;
	}

	fd = open(path, O_WRONLY | O_SYNC);
	if (fd < 0) {
		PR_ERROR("Unable to open %s\n", path);
		free(path);
		return;
	}

	rc = write(fd, &one, sizeof(one));
	if (rc < 0)
		PR_ERROR("Unable to write to %s\n", path);

	free(path);
	close(fd);
}

int kernel_fsi_probe(struct pdbg_target *target)
{
	struct fsi *fsi = target_to_fsi(target);
	int tries = 5;
	int rc;
	const char *kernel_path = kernel_get_fsi_path();
	const char *fsi_path;
	char *path;
	static bool first_probe = true;

	if (!kernel_path)
		return -1;

	fsi_path = pdbg_target_property(target, "device-path", NULL);
	assert(fsi_path);

	printf("kernel_fsi_probe fsi_path set is %s \n", fsi_path);
	rc = asprintf(&path, "%s%s", kernel_get_fsi_path(), fsi_path);
	if (rc < 0) {
		PR_ERROR("Unable to create fsi path\n");
		return rc;
	}

	while (tries) {
		fsi->fd = open(path, O_RDWR | O_SYNC);
		if (fsi->fd >= 0) {
			free(path);
			first_probe = false;
			return 0;
		}
		tries--;

		/*
		 * On fsi bus rescan, kernel re-creates all the slave device
		 * entries.  It means any currently open devices will be
		 * invalid and need to be re-opened.  So avoid scanning if
		 * some devices are already probed.
		 */
		if (first_probe) {
			kernel_fsi_scan_devices();
			sleep(1);
		} else {
			break;
		}
	}

	PR_INFO("Unable to open %s\n", path);
	free(path);
	return -1;
}

static void kernel_fsi_release(struct pdbg_target *target)
{
	struct fsi *fsi = target_to_fsi(target);

	if (fsi->fd != -1) {
		close(fsi->fd);
		fsi->fd = -1;
	}
}

static struct fsi kernel_fsi = {
	.target = {
		.name = "Kernel based FSI master",
		.compatible = "ibm,kernel-fsi",
		.class = "fsi",
		.probe = kernel_fsi_probe,
		.release = kernel_fsi_release,
	},
	.read = kernel_fsi_getcfam,
	.write = kernel_fsi_putcfam,
};
DECLARE_HW_UNIT(kernel_fsi);

static int kernel_pib_getscom(struct pib *pib, uint64_t addr, uint64_t *value)
{
	int rc;

	rc = pread(pib->fd, value, 8, addr);
	if (rc < 0) {
		rc = errno;
		PR_DEBUG("Failed to read scom addr 0x%016"PRIx64"\n", addr);
		return rc;
	}
	return 0;
}

static int kernel_pib_putscom(struct pib *pib, uint64_t addr, uint64_t value)
{
	int rc;

	rc = pwrite(pib->fd, &value, 8, addr);
	if (rc < 0) {
		rc = errno;
		PR_DEBUG("Failed to write scom addr 0x%016"PRIx64"\n", addr);
		return rc;
	}
	return 0;
}

static int kernel_pib_probe(struct pdbg_target *target)
{
	struct pib *pib = target_to_pib(target);
	const char *scom_path;

	scom_path = pdbg_target_property(target, "device-path", NULL);
	assert(scom_path);

	pib->fd = open(scom_path, O_RDWR | O_SYNC);
	if (pib->fd < 0) {
		PR_INFO("Unable to open %s\n", scom_path);
		return -1;
	}

	lseek(pib->fd, 0, SEEK_SET);
	return 0;
}

struct pdbg_target* get_ody_pib_target(struct pdbg_target *target)
{
	//TODO need to assert if the target passed is not of ocmb type
	uint32_t ocmb_proc = pdbg_target_index(pdbg_target_parent("proc", target));
	uint32_t ocmb_index = pdbg_target_index(target) % 0x8;

	struct pdbg_target *pib = NULL;
	struct pdbg_target *ody_target;
	pdbg_for_each_class_target("pib", ody_target) {
		uint32_t index = pdbg_target_index(ody_target);
		uint32_t proc = 0;
		if(!pdbg_target_u32_property(ody_target, "proc", &proc)) {
			if(index == ocmb_index && proc == ocmb_proc) {
				pib = ody_target;
				break;
			}
		}
	}
	assert(pib);

	return pib;
}

struct pdbg_target* get_ody_fsi_target(struct pdbg_target *target)
{
	//TODO need to assert if the target passed is not of ocmb type
	uint32_t ocmb_proc = pdbg_target_index(pdbg_target_parent("proc", target));
	uint32_t ocmb_index = pdbg_target_index(target) % 0x8;

	printf("get_ody_fsi_target ocmb_proc %d ocmb_index %d \n", ocmb_proc, ocmb_index);
	struct pdbg_target *fsi = NULL;
	struct pdbg_target *fsi_target;
	pdbg_for_each_class_target("fsi", fsi_target) {
		uint32_t index = pdbg_target_index(fsi_target);
		uint32_t proc = 0;
		if(!pdbg_target_u32_property(fsi_target, "proc", &proc)) {
			if(index == ocmb_index && proc == ocmb_proc) {
				fsi = fsi_target;
				break;
			}
		}
	}
	assert(fsi);

	return fsi;
}

struct pib kernel_pib = {
	.target = {
		.name =	"Kernel based FSI SCOM",
		.compatible = "ibm,kernel-pib",
		.class = "pib",
		.probe = kernel_pib_probe,
	},
	.read = kernel_pib_getscom,
	.write = kernel_pib_putscom,
};
DECLARE_HW_UNIT(kernel_pib);

__attribute__((constructor))
static void register_kernel(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &kernel_fsi_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &kernel_pib_hw_unit);
}
