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

/* Error codes */
#define EFSI 1

#define PRINT_ERR PR_DEBUG("%s: %d\n", __FUNCTION__, __LINE__)
#define CHECK_ERR(x) do {					\
	if (x) {	       					\
		PRINT_ERR;					\
		return x;					\
	}							\
	} while(0)

#define CHECK_ERR_GOTO(label, x) do {				\
	if (x) {	       					\
		PRINT_ERR;					\
		goto label;					\
	}							\
	} while(0)

#define FSI2PIB_BASE	0x1000

/* Alter display unit functions */
int adu_getmem(struct pdbg_target *target, uint64_t addr, uint8_t *output, uint64_t size, int ci);
int adu_putmem(struct pdbg_target *target, uint64_t start_addr, uint8_t *input, uint64_t size, int ci);

/* Functions to ram instructions */
#define THREAD_STATUS_DISABLED  PPC_BIT(0)
#define THREAD_STATUS_ACTIVE	PPC_BIT(63)
#define THREAD_STATUS_STATE	PPC_BITMASK(61, 62)
#define THREAD_STATUS_DOZE	PPC_BIT(62)
#define THREAD_STATUS_NAP	PPC_BIT(61)
#define THREAD_STATUS_SLEEP	PPC_BITMASK(61, 62)
#define THREAD_STATUS_QUIESCE	PPC_BIT(60)
#define THREAD_STATUS_SMT	PPC_BITMASK(57, 59)
#define THREAD_STATUS_SMT_1	PPC_BIT(59)
#define THREAD_STATUS_SMT_2SH	PPC_BIT(58)
#define THREAD_STATUS_SMT_2SP	(PPC_BIT(58) | PPC_BIT(59))
#define THREAD_STATUS_SMT_4	PPC_BIT(57)
#define THREAD_STATUS_SMT_8	(PPC_BIT(57) | PPC_BIT(59))


/* Opcodes for instruction ramming */
#define OPCODE_MASK  0xfc0003ffUL
#define MTNIA_OPCODE 0x00000002UL
#define MFNIA_OPCODE 0x00000004UL
#define MFMSR_OPCODE 0x7c0000a6UL
#define MTMSR_OPCODE 0x7c000124UL
#define MFSPR_OPCODE 0x7c0002a6UL
#define MTSPR_OPCODE 0x7c0003a6UL
#define LD_OPCODE 0xe8000000UL

/* GDB server functionality */
int gdbserver_start(uint16_t port);

enum fsi_system_type {FSI_SYSTEM_P8, FSI_SYSTEM_P9W, FSI_SYSTEM_P9R, FSI_SYSTEM_P9Z};
enum chip_type get_chip_type(uint64_t chip_id);

#endif
