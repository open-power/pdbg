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

#ifndef __LIBSBEFIFO_H__
#define __LIBSBEFIFO_H__

#include <stdint.h>

#define ESBEFIFO	201

#define SBEFIFO_PRI_SUCCESS           0x00000000
#define SBEFIFO_PRI_INVALID_COMMAND   0x00010000
#define SBEFIFO_PRI_INVALID_DATA      0x00020000
#define SBEFIFO_PRI_SEQUENCE_ERROR    0x00030000
#define SBEFIFO_PRI_INTERNAL_ERROR    0x00040000
#define SBEFIFO_PRI_UNKNOWN_ERROR     0x00FE0000

#define SBEFIFO_SEC_SUCCESS               0x0000
#define SBEFIFO_SEC_INVALID_CMD_CLASS     0x0001
#define SBEFIFO_SEC_INVALID_CMD           0x0002
#define SBEFIFO_SEC_INVALID_ADDRESS       0x0003
#define SBEFIFO_SEC_INVALID_TARGET        0x0004
#define SBEFIFO_SEC_INVALID_CHIPLET       0x0005
#define SBEFIFO_SEC_TARGET_NOT_PRESENT    0x0006
#define SBEFIFO_SEC_TARGET_NOT_FUNCTIONAL 0x0007
#define SBEFIFO_SEC_CMD_NOT_ALLOWED       0x0008
#define SBEFIFO_SEC_INVALID_FUNCTION      0x0009
#define SBEFIFO_SEC_GENERIC_FAILURE       0x000A
#define SBEFIFO_SEC_INVALID_ACCESS        0x000B
#define SBEFIFO_SEC_SBE_OS_FAIL           0x000C
#define SBEFIFO_SEC_SBE_ACCESS_FAIL       0x000D
#define SBEFIFO_SEC_MISSING_DATA          0x000E
#define SBEFIFO_SEC_EXTRA_DATA            0x000F
#define SBEFIFO_SEC_HW_TIMEOUT            0x0010
#define SBEFIFO_SEC_PIB_ERROR             0x0011
#define SBEFIFO_SEC_PARITY_ERROR          0x0012

struct sbefifo_context;

typedef int (*sbefifo_transport_fn)(uint8_t *msg, uint32_t msg_len,
				    uint8_t *out, uint32_t *out_len,
				    void *private_data);

int sbefifo_connect(const char *fifo_path, struct sbefifo_context **out);
int sbefifo_connect_transport(sbefifo_transport_fn transport, void *priv, struct sbefifo_context **out);
void sbefifo_disconnect(struct sbefifo_context *sctx);

int sbefifo_parse_output(struct sbefifo_context *sctx, uint32_t cmd,
			 uint8_t *buf, uint32_t buflen,
			 uint8_t **out, uint32_t *out_len);
int sbefifo_operation(struct sbefifo_context *sctx,
		      uint8_t *msg, uint32_t msg_len,
		      uint8_t **out, uint32_t *out_len);

uint32_t sbefifo_ffdc_get(struct sbefifo_context *sctx, const uint8_t **ffdc, uint32_t *ffdc_len);
void sbefifo_ffdc_dump(struct sbefifo_context *sctx);

int sbefifo_istep_execute(struct sbefifo_context *sctx, uint8_t major, uint8_t minor);

#define SBEFIFO_SCOM_OPERAND_NONE        0
#define SBEFIFO_SCOM_OPERAND_OR          1
#define SBEFIFO_SCOM_OPERAND_AND         2
#define SBEFIFO_SCOM_OPERAND_XOR         3

int sbefifo_scom_get(struct sbefifo_context *sctx, uint64_t addr, uint64_t *value);
int sbefifo_scom_put(struct sbefifo_context *sctx, uint64_t addr, uint64_t value);
int sbefifo_scom_modify(struct sbefifo_context *sctx, uint64_t addr, uint64_t value, uint8_t operand);
int sbefifo_scom_put_mask(struct sbefifo_context *sctx, uint64_t addr, uint64_t value, uint64_t mask);

int sbefifo_ring_get(struct sbefifo_context *sctx, uint32_t ring_addr, uint32_t ring_len_bits, uint16_t flags, uint8_t **ring_data, uint32_t *ring_len);
int sbefifo_ring_put(struct sbefifo_context *sctx, uint16_t ring_mode, uint8_t *ring_data, uint32_t ring_data_len);
int sbefifo_ring_put_from_image(struct sbefifo_context *sctx, uint16_t target, uint8_t chiplet_id, uint16_t ring_id, uint16_t ring_mode);

#define SBEFIFO_MEMORY_FLAG_PROC         0x0001
#define SBEFIFO_MEMORY_FLAG_PBA          0x0002
#define SBEFIFO_MEMORY_FLAG_AUTO_INCR    0x0004
#define SBEFIFO_MEMORY_FLAG_ECC_REQ      0x0008
#define SBEFIFO_MEMORY_FLAG_TAG_REQ      0x0010
#define SBEFIFO_MEMORY_FLAG_FAST_MODE    0x0020
#define SBEFIFO_MEMORY_FLAG_LCO_MODE     0x0040 // only for mem_put
#define SBEFIFO_MEMORY_FLAG_CI           0x0080
#define SBEFIFO_MEMORY_FLAG_PASSTHRU     0x0100
#define SBEFIFO_MEMORY_FLAG_CACHEINJECT  0x0200 // only for mem_put

int sbefifo_mem_get(struct sbefifo_context *sctx, uint64_t addr, uint32_t size, uint16_t flags, uint8_t **data);
int sbefifo_mem_put(struct sbefifo_context *sctx, uint64_t addr, uint8_t *data, uint32_t len, uint16_t flags);
int sbefifo_occsram_get(struct sbefifo_context *sctx, uint32_t addr, uint32_t size, uint8_t mode, uint8_t **data, uint32_t *data_len);
int sbefifo_occsram_put(struct sbefifo_context *sctx, uint32_t addr, uint8_t *data, uint32_t data_len, uint8_t mode);

#define SBEFIFO_REGISTER_TYPE_GPR	0x0
#define SBEFIFO_REGISTER_TYPE_SPR	0x1
#define SBEFIFO_REGISTER_TYPE_FPR	0x2

int sbefifo_register_get(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint8_t *reg_id, uint8_t reg_count, uint64_t **value);
int sbefifo_register_put(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t reg_type, uint8_t *reg_id, uint8_t reg_count, uint64_t *value);

int sbefifo_control_fast_array(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id, uint8_t mode, uint64_t clock_cycle);
int sbefifo_control_trace_array(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id, uint16_t array_id, uint16_t operation, uint8_t **trace_data, uint32_t *trace_data_len);

#define SBEFIFO_INSN_OP_START            0x0
#define SBEFIFO_INSN_OP_STOP             0x1
#define SBEFIFO_INSN_OP_STEP             0x2
#define SBEFIFO_INSN_OP_SRESET           0x3

int sbefifo_control_insn(struct sbefifo_context *sctx, uint8_t core_id, uint8_t thread_id, uint8_t thread_op, uint8_t mode);

int sbefifo_get_ffdc(struct sbefifo_context *sctx, uint8_t **ffdc, uint32_t *ffdc_len);
int sbefifo_get_capabilities(struct sbefifo_context *sctx, uint32_t *version, char **commit_id, char **release_tag, uint32_t **caps, uint32_t *caps_count);
int sbefifo_quiesce(struct sbefifo_context *sctx);

int sbefifo_mpipl_enter(struct sbefifo_context *sctx);
int sbefifo_mpipl_continue(struct sbefifo_context *sctx);
int sbefifo_mpipl_stopclocks(struct sbefifo_context *sctx, uint16_t target_type, uint8_t chiplet_id);


#endif /* __LIBSBEFIFO_H__ */
