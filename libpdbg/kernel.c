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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <err.h>
#include <inttypes.h>

#include "bitutils.h"
#include "operations.h"
#include "target.h"

#define FSI_SCAN_PATH "/sys/devices/platform/fsi-master/scan"
#define FSI_CFAM_PATH "/sys/devices/platform/fsi-master/slave@00:00/raw"

int fsi_fd;

static int kernel_fsi_getcfam(struct target *target, uint64_t addr, uint64_t *value)
{
	int rc;

	rc = lseek(fsi_fd, addr, SEEK_SET);
	if (rc < 0) {
		warn("Failed to seek %s", FSI_CFAM_PATH);
		return errno;
	}

	rc = read(fsi_fd, value, sizeof(*value));
	if (rc < 0) {
		warn("Failed to read from 0x%016"PRIx64, addr);
		return errno;
	}

	return 0;
}

static int kernel_fsi_putcfam(struct target *target, uint64_t addr, uint64_t data)
{
	int rc;

	rc = lseek(fsi_fd, addr, SEEK_SET);
	if (rc < 0) {
		warn("Failed to seek %s", FSI_CFAM_PATH);
		return errno;
	}

	rc = write(fsi_fd, &data, sizeof(data));
	if (rc < 0) {
		warn("Failed to write to 0x%016"PRIx64, addr);
		return errno;
	}

	return 0;
}

static void kernel_fsi_destroy(struct target *target)
{
	close(fsi_fd);
}

static void kernel_fsi_scan_devices(void)
{
	const char one = '1';
	int rc, fd;

	fd = open(FSI_SCAN_PATH, O_WRONLY | O_SYNC);
	if (fd < 0)
		err(errno, "Unable to open %s", FSI_SCAN_PATH);

	rc = write(fd, &one, sizeof(one));
	if (rc < 0)
		err(errno, "Unable to write to %s", FSI_SCAN_PATH);

	close(fd);
}

int kernel_fsi_target_init(struct target *target, const char *name,
			enum fsi_system_type type, struct target *next)
{
	uint64_t value;

	if (!fsi_fd) {
		int tries = 5;
		/* Scan */
		kernel_fsi_scan_devices();

		while (tries) {
			/* Open first raw device */
			fsi_fd = open(FSI_CFAM_PATH, O_RDWR | O_SYNC);
			if (fsi_fd >= 0)
				goto found;
			tries--;
			sleep(1);
		}
		if (fsi_fd < 0)
			err(errno, "Unable to open %s", FSI_CFAM_PATH);
	}
found:
	/* No cascaded devices after this one. */
	assert(next == NULL);
	target_init(target, name, 0, kernel_fsi_getcfam, kernel_fsi_putcfam,
		    kernel_fsi_destroy, next);

	/* Read chip id */
	CHECK_ERR(read_target(target, 0xc24, &value));
	target->chip_type = get_chip_type(value);

	return 0;
}
