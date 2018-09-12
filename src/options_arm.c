/* Copyright 2017 IBM Corp.
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
#include <string.h>
#include <unistd.h>

#include "main.h"

#define AMI_BMC "/proc/ractrends/Helper/FwInfo"
#define OPENFSI_BMC "/sys/bus/platform/devices/gpio-fsi/fsi0/"
#define FSI_CFAM_ID "/sys/devices/platform/gpio-fsi/fsi0/slave@00:00/cfam_id"

#define CHIP_ID_P8  0xea
#define CHIP_ID_P9  0xd1
#define CHIP_ID_P8P 0xd3

enum backend default_backend(void)
{
	int rc;
	rc = access(AMI_BMC, F_OK);
	if (rc == 0) /* AMI BMC */
		return I2C;

	rc = access(OPENFSI_BMC, F_OK);
	if (rc == 0) /* Kernel interface. OpenBMC */
		return KERNEL;

	return FAKE;
}

void print_backends(FILE *stream)
{
	fprintf(stream, "Valid backends: i2c kernel fsi fake\n");
}

void print_targets(FILE *stream)
{
	fprintf(stream, "kernel: No target is necessary\n");
	fprintf(stream, "i2c: No target is necessary\n");
	fprintf(stream, "fsi: p8 p9w p9r p9z\n");
}

static const char *default_kernel_target(void)
{
	FILE *cfam_id_file;

	/* Try and determine the correct device type */
	cfam_id_file = fopen(FSI_CFAM_ID, "r");
	if (cfam_id_file) {
		uint32_t cfam_id = 0;

		fscanf(cfam_id_file, "0x%" PRIx32, &cfam_id);
		fclose(cfam_id_file);

		switch((cfam_id >> 4) & 0xff) {
		case CHIP_ID_P9:
			return "p9";
			break;

		case CHIP_ID_P8:
		case CHIP_ID_P8P:
			return "p8";
			break;

		default:
			pdbg_log(PDBG_ERROR, "Unknown chip-id detected\n");
			pdbg_log(PDBG_ERROR, "You will need to specify a host type with -d <host type>\n");
			return NULL;
		}
	} else {
		/* The support for POWER8 included the cfam_id
		 * so if it doesn't exist assume we must be on
		 * P9 */
		pdbg_log(PDBG_WARNING, "Unable to determine host type, defaulting to p9\n");
		return "p9";
	}

	return NULL;
}

static const char *default_fsi_target(void)
{
	FILE *dt_compatible;
	char line[256];
	char *p;

	dt_compatible = fopen("/proc/device-tree/compatible", "r");
	if (!dt_compatible)
		return NULL;

	p = fgets(line, sizeof(line), dt_compatible);
	fclose(dt_compatible);
	if (!p) /* Uh oh*/
		return NULL;

	if (strstr(line, "witherspoon"))
		return "p9w";

	if (strstr(line, "romulus"))
		return "p9r";

	if (strstr(line, "zaius"))
		return "p9z";

	if (strstr(line, "palmetto"))
		return "p8";

	return NULL;
}

const char *default_target(enum backend backend)
{
	switch(backend) {
	case I2C:
		return NULL;
		break;

	case KERNEL:
		return default_kernel_target();
		break;

	case FSI:
		return default_fsi_target();
		break;

	default:
		return NULL;
		break;
	}

	return NULL;
}
