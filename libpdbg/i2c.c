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
#include <endian.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "bitutils.h"
#include "operations.h"
#include "debug.h"
#include "hwunit.h"

struct i2c_data {
	int addr;
	int fd;
};

static int i2c_set_addr(int fd, int addr)
{
	if (ioctl(fd, I2C_SLAVE, addr) < 0) {
		PR_ERROR("Unable to set i2c slave address\n");
		return -1;
	}

	return 0;
}

static int i2c_set_scom_addr(struct i2c_data *i2c_data, uint32_t addr)
{
	uint8_t data[4];

	addr <<= 1;
	data[3] = GETFIELD(PPC_BITMASK32(0, 7), addr);
	data[2] = GETFIELD(PPC_BITMASK32(8, 15), addr);
	data[1] = GETFIELD(PPC_BITMASK32(16, 23), addr);
	data[0] = GETFIELD(PPC_BITMASK32(23, 31), addr);
	if (write(i2c_data->fd, data, sizeof(data)) != 4) {
		PR_ERROR("Error writing address bytes\n");
		return -1;
	}

	return 0;
}

static int i2c_getscom(struct pib *pib, uint64_t addr, uint64_t *value)
{
	struct i2c_data *i2c_data = pib->priv;
	uint64_t data;

	CHECK_ERR(i2c_set_scom_addr(i2c_data, addr));

	if (read(i2c_data->fd, &data, sizeof(data)) != 8) {
		PR_ERROR("Error reading data\n");
		return -1;
	}

	*value = le64toh(data);

	return 0;
}

static int i2c_putscom(struct pib *pib, uint64_t addr, uint64_t value)
{
	struct i2c_data *i2c_data = pib->priv;
	uint8_t data[12];

	/* Setup scom address */
	addr <<= 1;
	data[3] = GETFIELD(PPC_BITMASK32(0, 7), addr);
	data[2] = GETFIELD(PPC_BITMASK32(8, 15), addr);
	data[1] = GETFIELD(PPC_BITMASK32(16, 23), addr);
	data[0] = GETFIELD(PPC_BITMASK32(23, 31), addr);

	/* Add data value */
	data[11] = GETFIELD(PPC_BITMASK(0, 7), value);
	data[10] = GETFIELD(PPC_BITMASK(8, 15), value);
	data[9] = GETFIELD(PPC_BITMASK(16, 23), value);
	data[8] = GETFIELD(PPC_BITMASK(23, 31), value);
	data[7] = GETFIELD(PPC_BITMASK(32, 39), value);
	data[6] = GETFIELD(PPC_BITMASK(40, 47), value);
	data[5] = GETFIELD(PPC_BITMASK(48, 55), value);
	data[4] = GETFIELD(PPC_BITMASK(56, 63), value);

	/* Write value */
	if (write(i2c_data->fd, data, sizeof(data)) != 12) {
		PR_ERROR("Error writing data bytes\n");
		return -1;
	}

	return 0;
}

#if 0
/* TODO: At present we don't have a generic destroy method as there aren't many
 * use cases for it. So for the moment we can just let the OS close the file
 * descriptor on exit. */
static void i2c_destroy(struct pib *pib)
{
	struct i2c_data *i2c_data = pib->priv;

	close(i2c_data->fd);
	free(i2c_data);
}
#endif

/*
 * Initialise a i2c backend on the given bus at the given bus address.
 */
int i2c_target_probe(struct pdbg_target *target)
{
	struct pib *pib = target_to_pib(target);
	struct i2c_data *i2c_data;
	const char *bus;
	int addr;

	bus = pdbg_get_backend_option();

	if (!bus)
		bus = pdbg_target_property(&pib->target, "bus", NULL);

	if (!bus)
		bus = "/dev/i2c4";

	addr = pdbg_target_address(&pib->target, NULL);
	assert(addr);

	i2c_data = malloc(sizeof(*i2c_data));
	if (!i2c_data)
		exit(1);

	i2c_data->addr = addr;
	i2c_data->fd = open(bus, O_RDWR);
	if (i2c_data->fd < 0) {
		perror("Error opening bus");
		return -1;
	}

	if (i2c_set_addr(i2c_data->fd, addr) < 0)
		return -1;

	pib->priv = i2c_data;

	return 0;
}

static struct pib p8_i2c_pib = {
	.target = {
		.name =	"POWER8 I2C Slave",
		.compatible = "ibm,power8-i2c-slave",
		.class = "pib",
		.probe = i2c_target_probe,
	},
	.read = i2c_getscom,
	.write = i2c_putscom,
	.fd = -1,
};
DECLARE_HW_UNIT(p8_i2c_pib);

__attribute__((constructor))
static void register_i2c(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p8_i2c_pib_hw_unit);
}
