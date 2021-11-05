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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "smbus.h"
#include "libi2c.h"

struct i2c_context {
	char *path;
	int fd;
	int addr;
};

int i2c_open(const char *devpath, struct i2c_context **out)
{
	struct i2c_context *ctx;
	int fd, ret;

	if (!devpath || !out)
		return EINVAL;

	ctx = malloc(sizeof(struct i2c_context));
	if (!ctx)
		return ENOMEM;

	*ctx = (struct i2c_context) {
		.fd = -1,
		.addr = -1,
	};

	ctx->path = strdup(devpath);
	assert(ctx->path);

	fd = open(devpath, O_RDWR);
	if (fd < 0) {
		ret = errno;
		free(ctx);
		LOG("libi2c: open %s failed, rc=%d", devpath, ret);
		return ret;
	}

	ctx->fd = fd;
	LOG("libi2c: open %s\n", devpath);

	*out = ctx;
	return 0;
}

void i2c_close(struct i2c_context *ctx)
{
	LOG("libi2c: close %s\n", ctx->path);

	if (ctx->path) {
		free(ctx->path);
		ctx->path = NULL;
	}

	if (ctx->fd != -1) {
		close(ctx->fd);
		ctx->fd = -1;
	}

	free(ctx);
}

static int i2c_set_address(struct i2c_context *ctx, int addr)
{
	long addr_7bit;
	int ret;

	if (ctx->addr == addr)
		return 0;

	addr_7bit = addr >> 1;

	ret = ioctl(ctx->fd, I2C_SLAVE, addr_7bit);
	if (ret < 0)
		ret = errno;
	else
		ctx->addr = addr;

	LOG("libi2c: %s addr 0x%02x, rc=%d\n", ctx->path, addr_7bit, ret);

	return ret;
}

int i2c_readn(struct i2c_context *ctx,
	      uint8_t addr,
	      uint8_t reg,
	      uint16_t *len,
	      uint8_t *data)
{
	int ret;

	ret = i2c_set_address(ctx, addr);
	if (ret != 0)
		return ret;

	if (*len == 1) {
		uint8_t val;

		ret = smbus_read_1(ctx->fd, reg, &val);
		if (ret != 0)
			return ret;

		data[0] = val;
	} else if (*len == 2) {
		uint16_t val;

		ret = smbus_read_2(ctx->fd, reg, &val);
		if (ret != 0)
			return ret;

		memcpy(data, &val, 2);
	} else {
		ret = smbus_read_n(ctx->fd, reg, len, data);
	}

	LOG("libi2c: %s read reg=%u, len=%u, rc=%d\n", ctx->path, reg, *len, ret);

	return 0;
}


int i2c_writen(struct i2c_context *ctx,
	       uint8_t addr,
	       uint8_t reg,
	       uint8_t *data,
	       uint16_t len)
{
	int ret;

	ret = i2c_set_address(ctx, addr);
	if (ret != 0)
		return ret;

	if (len == 1) {
		uint8_t val = *data;

		ret = smbus_write_1(ctx->fd, reg, val);
	} else if (len == 2) {
		uint16_t val = *(uint16_t *)data;

		ret = smbus_write_2(ctx->fd, reg, val);
	} else {
		ret = smbus_write_n(ctx->fd, reg, len, data);
	}

	LOG("libi2c: %s write reg=%u, len=%u, rc=%d\n", ctx->path, reg, len, ret);

	return ret;
}

void i2c_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
