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
#include <linux/i2c-dev.h>

#include "bitutils.h"
#include "operations.h"

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

static int i2c_getcfam(struct scom_backend *backend, int processor_id,
		       uint32_t *value, uint32_t addr)
{
	PR_ERROR("TODO: cfam access not supported with i2c backend\n");

	return -1;
}

static int i2c_putcfam(struct scom_backend *backend, int processor_id,
		       uint32_t value, uint32_t addr)
{
	PR_ERROR("TODO: cfam access not supported with i2c backend\n");

	return -1;
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

static int i2c_getscom(struct scom_backend *backend, int processor_id,
		       uint64_t *value, uint32_t addr)
{
	struct i2c_data *i2c_data = backend->priv;
	uint8_t data[8];

	if (processor_id) {
		PR_ERROR("TODO: secondary processor access not supported with i2c backend\n");
		return -1;
	}

	CHECK_ERR(i2c_set_scom_addr(i2c_data, addr));

	if (read(i2c_data->fd, &data, sizeof(data)) != 8) {
		PR_ERROR("Error reading data\n");
		return -1;
	}

	*value = *((uint64_t *) data);

	return 0;
}

static int i2c_putscom(struct scom_backend *backend, int processor_id,
		       uint64_t value, uint32_t addr)
{
	struct i2c_data *i2c_data = backend->priv;
	uint8_t data[12];

	if (processor_id) {
		PR_ERROR("TODO: secondary processor access not supported with i2c backend\n");
		return -1;
	}

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

static void i2c_destroy(struct scom_backend *backend)
{
	struct i2c_data *i2c_data = backend->priv;

	close(i2c_data->fd);
	free(backend->priv);
}

/*
 * Initialise a i2c backend on the given bus at the given bus address.
 */
struct scom_backend *i2c_init(char *bus, int addr)
{
	struct scom_backend *backend;
	struct i2c_data *i2c_data;

	backend = malloc(sizeof(*backend));
	i2c_data = malloc(sizeof(*i2c_data));
	if (!backend || !i2c_data)
		exit(1);

	i2c_data->addr = addr;
	i2c_data->fd = open(bus, O_RDWR);
	if (i2c_data->fd < 0) {
		perror("Error opening bus");
		exit(1);
	}

	if (i2c_set_addr(i2c_data->fd, addr) < 0)
		exit(1);

	backend->getscom = i2c_getscom;
	backend->putscom = i2c_putscom;
	backend->getcfam = i2c_getcfam;
	backend->putcfam = i2c_putcfam;
	backend->destroy = i2c_destroy;
	backend->priv = i2c_data;

	return backend;
}
