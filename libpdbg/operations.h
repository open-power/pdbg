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

#include "target.h"

/* Error codes */
#define EFSI 1

#define PR_DEBUG(x, args...) \
	//fprintf(stderr, x, ##args)
#define PR_INFO(x, args...) \
	fprintf(stderr, x, ##args)
#define PR_ERROR(x, args...) \
	fprintf(stderr, "%s: " x, __FUNCTION__, ##args)

#define CHECK_ERR(x) do {					\
	if (x) {	       					\
		PR_DEBUG("%s: %d\n", __FUNCTION__, __LINE__);	\
		return -EFSI;					\
	}							\
	} while(0)

#define THREADS_PER_CORE	8

#define FSI2PIB_BASE	0x1000

/* Alter display unit functions */
int adu_getmem(struct target *target, uint64_t addr, uint8_t *output, uint64_t size);
int adu_putmem(struct target *target, uint64_t start_addr, uint8_t *input, uint64_t size);

/* Functions to ram instructions */
#define THREAD_STATUS_DISABLED  PPC_BIT(0)
#define THREAD_STATUS_ACTIVE	PPC_BIT(63)
#define THREAD_STATUS_STATE	PPC_BITMASK(61, 62)
#define THREAD_STATUS_DOZE	PPC_BIT(62)
#define THREAD_STATUS_NAP	PPC_BIT(61)
#define THREAD_STATUS_SLEEP	PPC_BITMASK(61, 62)
#define THREAD_STATUS_QUIESCE	PPC_BIT(60)

int ram_getgpr(struct target *thread, int gpr, uint64_t *value);
int ram_putgpr(struct target *thread, int gpr, uint64_t value);
int ram_getnia(struct target *thread, uint64_t *value);
int ram_putnia(struct target *thread, uint64_t value);
int ram_getspr(struct target *thread, int spr, uint64_t *value);
int ram_putspr(struct target *thread, int spr, uint64_t value);
int ram_getmsr(struct target *thread, uint64_t *value);
int ram_putmsr(struct target *thread, uint64_t value);
int ram_getmem(struct target *thread, uint64_t addr, uint64_t *value);
uint64_t chiplet_thread_status(struct target *thread);
int ram_stop_thread(struct target *thread);
int ram_step_thread(struct target *thread, int count);
int ram_start_thread(struct target *thread);

/* GDB server functionality */
int gdbserver_start(uint16_t port);

enum fsi_system_type {FSI_SYSTEM_P8, FSI_SYSTEM_P9W, FSI_SYSTEM_P9R, FSI_SYSTEM_P9Z};
int fsi_target_init(struct target *target, const char *name, enum fsi_system_type tpye, struct target *next);
int fsi_target_probe(struct target *targets, int max_target_count);
int i2c_target_init(struct target *target, const char *name, struct target *next,
		    const char *bus, int addr);
int fsi2pib_target_init(struct target *target, const char *name, uint64_t base, struct target *next);
int kernel_fsi2pib_target_init(struct target *target, const char *name, uint64_t base, struct target *next);
int opb_target_init(struct target *target, const char *name, uint64_t base, struct target *next);
enum chip_type get_chip_type(uint64_t chip_id);
int thread_target_init(struct target *thread, const char *name, uint64_t thread_id, struct target *next);
int thread_target_probe(struct target *chiplet, struct target *targets, int max_target_count);
int chiplet_target_init(struct target *target, const char *name, uint64_t chip_id, struct target *next);
int chiplet_target_probe(struct target *processor, struct target *targets, int max_target_count);
int hmfsi_target_probe(struct target *cfam, struct target *targets, int max_target_count);

#endif
