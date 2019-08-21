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
#ifndef __HWUNIT_H
#define __HWUNIT_H

#include <stdint.h>

#include "target.h"

/* This works and should be safe because struct pdbg_target is guaranteed to be
 * the first member of the specialised type (see the DECLARE_HW_UNIT definition
 * below). I'm not sure how sane it is though. Probably not very but it does
 * remove a bunch of tedious container_of() typing */
#define translate_cast(x) (uint64_t (*)(struct pdbg_target *, uint64_t)) (x)

struct hw_unit_info {
	void *hw_unit;
	size_t size;
};

void pdbg_hwunit_register(const struct hw_unit_info *hw_unit);
const struct hw_unit_info *pdbg_hwunit_find_compatible(const char *compat);

/*
 * If this macro fails to compile for you, you've probably not
 * declared the struct pdbg_target as the first member of the
 * container struct. Not doing so will break other assumptions.
 */
#define DECLARE_HW_UNIT(name)						\
	__attribute__((unused)) static inline void name ##_hw_unit_check(void) {	\
		((void)sizeof(char[1 - 2 * (container_off(typeof(name), target) != 0)])); \
	}								\
	const struct hw_unit_info __used name ##_hw_unit =              \
	{ .hw_unit = &name, .size = sizeof(name) };

struct htm {
	struct pdbg_target target;
	int (*start)(struct htm *);
	int (*stop)(struct htm *);
	int (*status)(struct htm *);
	int (*dump)(struct htm *, char *);
	int (*record)(struct htm *, char *);
};
#define target_to_htm(x) container_of(x, struct htm, target)

struct mem {
	struct pdbg_target target;
	int (*getmem)(struct mem *, uint64_t, uint64_t *, int, uint8_t);
	int (*putmem)(struct mem *, uint64_t, uint64_t, int, int, uint8_t);
	int (*read)(struct mem *, uint64_t, uint8_t *, uint64_t, uint8_t, bool);
	int (*write)(struct mem *, uint64_t, uint8_t *, uint64_t, uint8_t, bool);
};
#define target_to_mem(x) container_of(x, struct mem, target)

struct sbefifo {
	struct pdbg_target target;
	int (*istep)(struct sbefifo *, uint32_t major, uint32_t minor);
	int (*thread_start)(struct sbefifo *, uint32_t core_id, uint32_t thread_id);
	int (*thread_stop)(struct sbefifo *, uint32_t core_id, uint32_t thread_id);
	int (*thread_step)(struct sbefifo *, uint32_t core_id, uint32_t thread_id);
	int (*thread_sreset)(struct sbefifo *, uint32_t core_id, uint32_t thread_id);
	uint32_t (*ffdc_get)(struct sbefifo *, const uint8_t **, uint32_t *);
	int fd;
	uint32_t status;
	uint8_t *ffdc;
	uint32_t ffdc_len;
};
#define target_to_sbefifo(x) container_of(x, struct sbefifo, target)

struct pib {
	struct pdbg_target target;
	int (*read)(struct pib *, uint64_t, uint64_t *);
	int (*write)(struct pib *, uint64_t, uint64_t);
	void *priv;
	int fd;
};
#define target_to_pib(x) container_of(x, struct pib, target)

struct opb {
	struct pdbg_target target;
	int (*read)(struct opb *, uint32_t, uint32_t *);
	int (*write)(struct opb *, uint32_t, uint32_t);
};
#define target_to_opb(x) container_of(x, struct opb, target)

struct fsi {
	struct pdbg_target target;
	int (*read)(struct fsi *, uint32_t, uint32_t *);
	int (*write)(struct fsi *, uint32_t, uint32_t);
	enum chip_type chip_type;
};
#define target_to_fsi(x) container_of(x, struct fsi, target)

struct core {
	struct pdbg_target target;
	bool release_spwkup;
};
#define target_to_core(x) container_of(x, struct core, target)

struct thread {
	struct pdbg_target target;
	struct thread_state status;
	int id;
	int (*step)(struct thread *, int);
	int (*start)(struct thread *);
	int (*stop)(struct thread *);
	int (*sreset)(struct thread *);

	bool ram_did_quiesce; /* was the thread quiesced by ram mode */

	/* ram_setup() should be called prior to using ram_instruction() to
	 * actually ram the instruction and return the result. ram_destroy()
	 * should be called at completion to clean-up. */
	bool ram_is_setup;
	int (*ram_setup)(struct thread *);
	int (*ram_instruction)(struct thread *, uint64_t opcode, uint64_t *scratch);
	int (*ram_destroy)(struct thread *);
	int (*ram_getxer)(struct pdbg_target *, uint64_t *value);
	int (*ram_putxer)(struct pdbg_target *, uint64_t value);
	int (*enable_attn)(struct pdbg_target *);
};
#define target_to_thread(x) container_of(x, struct thread, target)

/* Place holder for chiplets which we just want translation for */
struct chiplet {
        struct pdbg_target target;
	int (*getring)(struct chiplet *, uint64_t, int64_t, uint32_t[]);
};
#define target_to_chiplet(x) container_of(x, struct chiplet, target)

struct xbus {
	struct pdbg_target target;
	uint32_t ring_id;
};
#define target_to_xbus(x) container_of(x, struct xbus, target)

#endif /* __HWUNIT_H */
