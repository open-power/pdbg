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
#ifndef __OPERATIONS_H
#define __OPERATIONS_H

#include "bitutils.h"
#include "target.h"
#include "debug.h"

#define PRINT_ERR PR_DEBUG("failed\n");
#define CHECK_ERR(x) do {					\
	typeof(x) __x = (x);					\
	if (__x) {	       					\
		PRINT_ERR;					\
		return __x;					\
	}							\
	} while(0)

#define CHECK_ERR_GOTO(label, x) do {				\
	if ((x)) {	       					\
		PRINT_ERR;					\
		goto label;					\
	}							\
	} while(0)

#define FSI2PIB_BASE	0x1000

/* Opcodes for instruction ramming */
#define OPCODE_MASK  0xfc0003ffUL
#define MTNIA_OPCODE 0x00000002UL
#define MFNIA_OPCODE 0x00000004UL
#define MFMSR_OPCODE 0x7c0000a6UL
#define MTMSR_OPCODE 0x7c000164UL
#define MFSPR_OPCODE 0x7c0002a6UL
#define MTSPR_OPCODE 0x7c0003a6UL
#define MFOCRF_OPCODE 0x7c100026UL
#define MTOCRF_OPCODE 0x7C100120UL
#define MFSPR_MASK (MFSPR_OPCODE | ((0x1f) << 16) | ((0x3e0) << 6))
#define MFXER_OPCODE (MFSPR_OPCODE | ((1 & 0x1f) << 16) | ((1 & 0x3e0) << 6))
#define MTXER_OPCODE (MTSPR_OPCODE | ((1 & 0x1f) << 16) | ((1 & 0x3e0) << 6))
#define LD_OPCODE 0xe8000000UL

#define MXSPR_SPR(opcode) (((opcode >> 16) & 0x1f) | ((opcode >> 6) & 0x3e0))

enum fsi_system_type {FSI_SYSTEM_P8, FSI_SYSTEM_P9W, FSI_SYSTEM_P9R, FSI_SYSTEM_P9Z};
enum chip_type get_chip_type(uint64_t chip_id);

#endif
