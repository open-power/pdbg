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

#ifndef __SBEFIFO_PRIVATE_H__
#define __SBEFIFO_PRIVATE_H__

#include <stdint.h>
#include "libsbefifo.h"

#define SBEFIFO_CMD_CLASS_CONTROL        0xA100
#define   SBEFIFO_CMD_EXECUTE_ISTEP        0x01

#define SBEFIFO_CMD_CLASS_SCOM           0xA200
#define   SBEFIFO_CMD_GET_SCOM             0x01
#define   SBEFIFO_CMD_PUT_SCOM             0x02
#define   SBEFIFO_CMD_MODIFY_SCOM          0x03
#define   SBEFIFO_CMD_PUT_SCOM_MASK        0x04
#define   SBEFIFO_CMD_MULTI_SCOM           0x05

#define SBEFIFO_CMD_CLASS_RING           0xA300
#define   SBEFIFO_CMD_GET_RING             0x01
#define   SBEFIFO_CMD_PUT_RING             0x02
#define   SBEFIFO_CMD_PUT_RING_IMAGE       0x03

#define SBEFIFO_CMD_CLASS_MEMORY         0xA400
#define   SBEFIFO_CMD_GET_MEMORY           0x01
#define   SBEFIFO_CMD_PUT_MEMORY           0x02
#define   SBEFIFO_CMD_GET_OCCSRAM          0x03
#define   SBEFIFO_CMD_PUT_OCCSRAM          0x04

#define SBEFIFO_CMD_CLASS_REGISTER       0xA500
#define   SBEFIFO_CMD_GET_REGISTER         0x01
#define   SBEFIFO_CMD_PUT_REGISTER         0x02

#define SBEFIFO_CMD_CLASS_ARRAY          0xA600
#define   SBEFIFO_CMD_FAST_ARRAY           0x01
#define   SBEFIFO_CMD_TRACE_ARRAY          0x02

#define SBEFIFO_CMD_CLASS_INSTRUCTION    0xA700
#define   SBEFIFO_CMD_CONTROL_INSN         0x01

#define SBEFIFO_CMD_CLASS_GENERIC        0xA800
#define   SBEFIFO_CMD_GET_FFDC             0x01
#define   SBEFIFO_CMD_GET_CAPABILITY       0x02
#define   SBEFIFO_CMD_QUIESCE              0x03

#define SBEFIFO_CMD_CLASS_MPIPL          0xA900
#define SBEFIFO_CMD_ENTER_MPIPL            0x01
#define SBEFIFO_CMD_CONTINUE_MPIPL         0x02
#define SBEFIFO_CMD_STOP_CLOCKS            0x03
#define SBEFIFO_CMD_GET_TI_INFO            0x04

struct sbefifo_context {
	int fd;

	sbefifo_transport_fn transport;
	void *priv;

	uint32_t status;
	uint8_t *ffdc;
	uint32_t ffdc_len;
};

void sbefifo_debug(const char *fmt, ...);

void sbefifo_ffdc_clear(struct sbefifo_context *sctx);
void sbefifo_ffdc_set(struct sbefifo_context *sctx, uint32_t status, uint8_t *ffdc, uint32_t ffdc_len);

int sbefifo_operation(struct sbefifo_context *sctx,
		      uint8_t *msg, uint32_t msg_len,
		      uint8_t **out, uint32_t *out_len);

#ifdef LIBSBEFIFO_DEBUG
#define LOG(fmt, args...)	sbefifo_debug(fmt, ##args)
#else
#define LOG(fmt, args...)
#endif

#endif /* __SBEFIFO_PRIVATE_H__ */
