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

#ifndef __LIBI2C_H__
#define __LIBI2C_H__

#include <stdint.h>

/*
 * libi2c implements access to i2c devices via /dev interface using the Linux
 * kernel driver.  This library is based on the I2C Tools package.
 */

struct i2c_context;

int i2c_open(const char *devpath, struct i2c_context **out);
void i2c_close(struct i2c_context *ctx);

int i2c_readn(struct i2c_context *ctx,
	      uint8_t addr,
	      uint8_t reg,
	      uint16_t *len,
	      uint8_t *data);

int i2c_writen(struct i2c_context *ctx,
	       uint8_t addr,
	       uint8_t reg,
	       uint8_t *data,
	       uint16_t len);

#endif /* __LIBI2C_H__ */
