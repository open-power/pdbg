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
#include <stdio.h>

#include <libi2c/libi2c.h>

#include "debug.h"
#include "hwunit.h"

static int kernel_i2c_read(struct i2cbus *i2cbus, uint8_t addr, uint8_t reg, uint16_t size, uint8_t *data)
{
	int rc;
	uint16_t len = size;

	rc = i2c_readn(i2cbus->ctx, addr, reg, &len, data);
	if (rc) {
		PR_DEBUG("i2c read failed, rc %d\n", rc);
		return -1;
	}

	PR_DEBUG("i2c read %u bytes\n", len);

	if (len != size) {
		PR_ERROR("i2c read mismatch, expected %u, got %u bytes\n", size, len);
		return -1;
	}

	return 0;
}

static int kernel_i2c_write(struct i2cbus *i2cbus, uint8_t addr, uint8_t reg, uint16_t size, uint8_t *data)
{
	int rc;

	rc = i2c_writen(i2cbus->ctx, addr, reg, data, size);
	if (rc) {
		PR_DEBUG("i2c write failed, rc %d\n", rc);
		return -1;
	}

	PR_DEBUG("i2c write %u bytes\n", size);

	return 0;
}

static int kernel_i2c_probe(struct pdbg_target *target)
{
	struct i2cbus *i2cbus = target_to_i2cbus(target);
	const char *i2c_path;
	int rc;

	i2c_path = pdbg_target_property(target, "device-path", NULL);
	assert(i2c_path);

	rc = i2c_open(i2c_path, &i2cbus->ctx);
	if (rc) {
		PR_WARNING("Unable to open i2c driver %s, rc=%d\n", i2c_path, rc);
		return -1;
	}

	return 0;
}

static void kernel_i2c_release(struct pdbg_target *target)
{
	struct i2cbus *i2cbus = target_to_i2cbus(target);

	i2c_close(i2cbus->ctx);
	i2cbus->ctx = NULL;
}

static struct i2cbus kernel_i2c_bus = {
	.target = {
		.name =	"Kernel I2C Bus",
		.compatible = "ibm,kernel-i2c-bus",
		.class = "i2c_bus",
		.probe = kernel_i2c_probe,
		.release = kernel_i2c_release,
	},
	.read = kernel_i2c_read,
	.write = kernel_i2c_write,
};
DECLARE_HW_UNIT(kernel_i2c_bus);

__attribute__((constructor))
static void register_i2c_bus(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &kernel_i2c_bus_hw_unit);
}
