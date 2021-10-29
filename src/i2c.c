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

#include <libpdbg.h>
#include <inttypes.h>
#include <stdlib.h>

#include "main.h"
#include "optcmd.h"
#include "path.h"
#include "target.h"
#include "util.h"
#include "debug.h"

static int geti2c(uint8_t addr, uint8_t reg, uint16_t size)
{
	uint8_t *data = NULL;
	struct pdbg_target *target;
	int n = 0, rc = 0;

	data = malloc(size);
	assert(data);

	for_each_path_target_class("i2c_bus", target) {
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			continue;

		rc = i2c_read(target, addr, reg, size, data);
		if (rc) {
			printf("%s: read failed\n", pdbg_target_path(target));
		} else {
			printf("%s: read %d byte(s): ", pdbg_target_path(target), size);
			if (size <= 8) {
				int i;

				for (i=0; i<size; i++)
					printf("0x%02x ", data[i]);
				printf("\n");
			} else {
				printf("\n");
				hexdump(0, data, size, 1);
			}
		}
		n++;
	}

	return n;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(geti2c, geti2c, (DATA8, DATA8, DATA16));

static int puti2c(uint8_t addr, uint8_t reg, uint16_t value)
{
	struct pdbg_target *target;
	uint16_t size;
	uint8_t data[2];
	int n = 0, rc = 0;

	if (value <= 0xFF) {
		size = 1;
		data[0] = value & 0xff;
	} else {
		size = 2;
		memcpy(data, &value, 2);
	}

	for_each_path_target_class("i2c_bus", target) {
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
			continue;

		rc = i2c_write(target, addr, reg, size, data);
		if (rc) {
			printf("%s: write failed\n", pdbg_target_path(target));
		} else {
			printf("%s: write %d bytes\n", pdbg_target_path(target), size);
		}
		n++;
	}

	return n;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(puti2c, puti2c, (DATA8, DATA8, DATA16));
