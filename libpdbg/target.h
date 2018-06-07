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
#ifndef __TARGET_H
#define __TARGET_H

#include <stdint.h>
#include <ccan/list/list.h>
#include <ccan/str/str.h>
#include <ccan/container_of/container_of.h>
#include "compiler.h"
#include "device.h"
#include "libpdbg.h"

enum chip_type {CHIP_UNKNOWN, CHIP_P8, CHIP_P8NV, CHIP_P9};

struct pdbg_target_class {
	char *name;
	struct list_head targets;
	struct list_node class_head_link;
};

struct pdbg_target {
	char *name;
	char *compatible;
	char *class;
	int (*probe)(struct pdbg_target *target);
	void (*release)(struct pdbg_target *target);
	int index;
	enum pdbg_target_status status;
	const char *dn_name;
	struct list_node list;
	struct list_head properties;
	struct list_head children;
	struct pdbg_target *parent;
	u32 phandle;
	bool probed;
	struct list_node class_link;
	void *priv;
};

struct pdbg_target *require_target_parent(struct pdbg_target *target);
struct pdbg_target_class *find_target_class(const char *name);
struct pdbg_target_class *require_target_class(const char *name);
struct pdbg_target_class *get_target_class(const char *name);
bool pdbg_target_is_class(struct pdbg_target *target, const char *class);

extern struct list_head empty_list;
extern struct list_head target_classes;

#define for_each_class_target(class_name, target)			\
	list_for_each((find_target_class(class_name) ? &require_target_class(class_name)->targets : &empty_list), target, class_link)

#define for_each_target_class(target_class)			\
	list_for_each(&target_classes, target_class, class_head_link)

struct hw_unit_info {
	void *hw_unit;
	size_t size;
};
struct hw_unit_info *find_compatible_target(const char *compat);

/* We can't pack the structs themselves directly into a special
 * section because there doesn't seem to be any standard way of doing
 * that due to alignment rules. So instead we pack pointers into a
 * special section.
 *
 * If this macro fails to compile for you, you've probably not
 * declared the struct pdbg_target as the first member of the
 * container struct. Not doing so will break other assumptions.
 * */
#define DECLARE_HW_UNIT(name)						\
	static inline void name ##_hw_unit_check(void) {		\
		((void)sizeof(char[1 - 2 * (container_off(typeof(name), target) != 0)])); \
	}								\
	const struct hw_unit_info __used name ##_hw_unit =              \
	{ .hw_unit = &name, .size = sizeof(name) }; 	                \
	const struct hw_unit_info __used __section("hw_units") *name ##_hw_unit_p = &name ##_hw_unit

struct htm {
	struct pdbg_target target;
	int (*start)(struct htm *);
	int (*stop)(struct htm *);
	int (*reset)(struct htm *, uint64_t *, uint64_t *);
	int (*status)(struct htm *);
	int (*dump)(struct htm *, uint64_t, const char *);
};
#define target_to_htm(x) container_of(x, struct htm, target)

struct adu {
	struct pdbg_target target;
	int (*getmem)(struct adu *, uint64_t, uint64_t *, int);
	int (*putmem)(struct adu *, uint64_t, uint64_t, int, int);
};
#define target_to_adu(x) container_of(x, struct adu, target)

struct pib {
	struct pdbg_target target;
	int (*read)(struct pib *, uint64_t, uint64_t *);
	int (*write)(struct pib *, uint64_t, uint64_t);
	void *priv;
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
};
#define target_to_thread(x) container_of(x, struct thread, target)

/* Place holder for chiplets which we just want translation for */
struct chiplet {
        struct pdbg_target target;
	int (*getring)(struct chiplet *, uint64_t, int64_t, uint32_t[]);
};
#define target_to_chiplet(x) container_of(x, struct chiplet, target)
#endif
