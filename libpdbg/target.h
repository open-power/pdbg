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
#include <ccan/short_types/short_types.h>
#include "compiler.h"
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
	const char* path;
	int (*probe)(struct pdbg_target *target);
	void (*release)(struct pdbg_target *target);
	uint64_t (*translate)(struct pdbg_target *target, uint64_t addr);
	void *fdt;
	int fdt_offset;
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
	struct pdbg_target *vnode;
};

struct pdbg_mfile {
	int fd;
	ssize_t len;
	void *fdt;
	bool readonly;
};

struct pdbg_dtb {
	struct pdbg_mfile backend;
	struct pdbg_mfile system;
};

struct pdbg_target *get_parent(struct pdbg_target *target, bool system);
struct pdbg_target *target_parent(const char *klass, struct pdbg_target *target, bool system);
struct pdbg_target *require_target_parent(const char *klass, struct pdbg_target *target, bool system);
struct pdbg_target_class *find_target_class(const char *name);
struct pdbg_target_class *require_target_class(const char *name);
struct pdbg_target_class *get_target_class(struct pdbg_target *target);
bool pdbg_target_is_class(struct pdbg_target *target, const char *class);

extern struct list_head empty_list;
extern struct list_head target_classes;

struct pdbg_dtb *pdbg_default_dtb(void *system_fdt);
enum pdbg_backend pdbg_get_backend(void);
const char *pdbg_get_backend_option(void);
bool pdbg_fdt_is_readonly(void *fdt);

struct chipop *pib_to_chipop(struct pdbg_target *target);
bool target_is_virtual(struct pdbg_target *target);
struct pdbg_target *target_to_real(struct pdbg_target *target, bool strict);
struct pdbg_target *target_to_virtual(struct pdbg_target *target, bool strict);

#endif
