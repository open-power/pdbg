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

#ifndef __SMBUS_H__
#define __SMBUS_H__

#include <stdint.h>

int smbus_read_1(int fd, uint8_t cmd, uint8_t *out);
int smbus_read_2(int fd, uint8_t cmd, uint16_t *out);
int smbus_read_n(int fd, uint8_t cmd, uint16_t *len, uint8_t *out);

int smbus_write_1(int fd, uint8_t cmd, uint8_t in);
int smbus_write_2(int fd, uint8_t cmd, uint16_t in);
int smbus_write_n(int fd, uint8_t cmd, uint16_t len, uint8_t *in);

void i2c_debug(const char *fmt, ...);

#ifdef LIBI2C_DEBUG
#define LOG(fmt, args...)	i2c_debug(fmt, ##args)
#else
#define LOG(fmt, args...)
#endif

#endif /* __SMBUS_H__ */
