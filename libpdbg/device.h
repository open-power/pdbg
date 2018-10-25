/* Copyright 2013-2014 IBM Corp.
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

#ifndef __DEVICE_H
#define __DEVICE_H
#include <ccan/list/list.h>
#include <ccan/short_types/short_types.h>
#include "compiler.h"

/* Any property or node with this prefix will not be passed to the kernel. */
#define DT_PRIVATE	"skiboot,"

/*
 * An in-memory representation of a node in the device tree.
 *
 * This is trivially flattened into an fdt.
 *
 * Note that the add_* routines will make a copy of the name if it's not
 * a read-only string (ie. usually a string literal).
 */
struct dt_property {
	struct list_node list;
	const char *name;
	size_t len;
	char prop[/* len */];
};

extern struct pdbg_target *dt_root;

/* Add a property node, various forms. */
struct dt_property *dt_add_property(struct pdbg_target *node,
				    const char *name,
				    const void *val, size_t size);

/* Warning: moves *prop! */
void dt_resize_property(struct dt_property **prop, size_t len);

/* Check a compatible property */
bool dt_node_is_compatible(const struct pdbg_target *node, const char *compat);

/* Find a node based on compatible property */
struct pdbg_target *dt_find_compatible_node(struct pdbg_target *root,
					struct pdbg_target *prev,
					const char *compat);

#define dt_for_each_compatible(root, node, compat)	\
	for (node = NULL; 			        \
	     (node = dt_find_compatible_node(root, node, compat)) != NULL;)

/* Find a property by name. */
struct dt_property *dt_find_property(const struct pdbg_target *node,\
				     const char *name);

/* Simplified accessors */
u32 dt_prop_get_u32(const struct pdbg_target *node, const char *prop);
u32 dt_prop_get_u32_index(const struct pdbg_target *node, const char *prop, u32 index);
const void *dt_prop_get(const struct pdbg_target *node, const char *prop);
const void *dt_prop_get_def(const struct pdbg_target *node, const char *prop,
			    void *def);

/* Parsing helpers */
u64 dt_get_number(const void *pdata, unsigned int cells);

/* Find an chip-id property in this node; if not found, walk up the parent
 * nodes. Returns -1 if no chip-id property exists. */
u32 dt_get_chip_id(const struct pdbg_target *node);

/* Address accessors ("reg" properties parsing). No translation,
 * only support "simple" address forms (1 or 2 cells). Asserts
 * if address doesn't exist
 */
u64 dt_get_address(const struct pdbg_target *node, unsigned int index,
		   u64 *out_size);

#endif /* __DEVICE_H */
