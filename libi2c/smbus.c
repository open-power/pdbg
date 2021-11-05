/* Copyright 2021 IBM Corp.
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
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "smbus.h"

static int smbus_op(int fd, char read_write, uint8_t cmd, int len,
		    union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;
	int ret;

	args.read_write = read_write;
	args.command = cmd;
	args.size = len;
	args.data = data;

	ret = ioctl(fd, I2C_SMBUS, &args);
	if (ret == -1)
		ret = errno;

	LOG("libi2c: smbus_op: rw:%d, cmd:%u, size=%d, rc=%d\n",
	    read_write, cmd, len, ret);

	return ret;
}

int smbus_read_1(int fd, uint8_t cmd, uint8_t *out)
{
	union i2c_smbus_data data;
	int ret;

	ret = smbus_op(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_BYTE_DATA, &data);
	if (ret != 0)
		return ret;

	*out = data.byte & 0xff;
	return 0;
}

int smbus_read_2(int fd, uint8_t cmd, uint16_t *out)
{
	union i2c_smbus_data data;
	int ret;

	ret = smbus_op(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_WORD_DATA, &data);
	if (ret != 0)
		return ret;

	*out = data.word & 0xffff;
	return 0;
}

int smbus_read_n(int fd, uint8_t cmd, uint16_t *len, uint8_t *out)
{
	union i2c_smbus_data data;
	int ret, i;

	if (*len > I2C_SMBUS_BLOCK_MAX)
		return EINVAL;

	if (*len == 0) {
		ret = smbus_op(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_BLOCK_DATA, &data);
	} else {
		data.block[0] = *len;

		ret = smbus_op(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_I2C_BLOCK_DATA, &data);
	}
	if (ret != 0)
		return ret;

	*len = data.block[0];
	for (i = 1; i <= data.block[0]; i++)
		out[i-1] = data.block[i];

	return 0;
}

int smbus_write_1(int fd, uint8_t cmd, uint8_t in)
{
	union i2c_smbus_data data;

	data.byte = in;
	return smbus_op(fd, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_BYTE_DATA, &data);
}

int smbus_write_2(int fd, uint8_t cmd, uint16_t in)
{
	union i2c_smbus_data data;

	data.word = in;
	return smbus_op(fd, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_WORD_DATA, &data);
}

int smbus_write_n(int fd, uint8_t cmd, uint16_t len, uint8_t *in)
{
	union i2c_smbus_data data;
	int i;

	if (len > I2C_SMBUS_BLOCK_MAX)
		return EINVAL;

	data.block[0] = len;
	for (i = 1; i <= len; i++)
		data.block[i] = in[i-1];

	return smbus_op(fd, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_BLOCK_DATA, &data);
}
