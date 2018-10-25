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

extern struct pdbg_target *dt_root;

/* Check a compatible property */
bool dt_node_is_compatible(const struct pdbg_target *node, const char *compat);

/* Find a node based on compatible property */
struct pdbg_target *dt_find_compatible_node(struct pdbg_target *root,
					struct pdbg_target *prev,
					const char *compat);

#define dt_for_each_compatible(root, node, compat)	\
	for (node = NULL; 			        \
	     (node = dt_find_compatible_node(root, node, compat)) != NULL;)

#endif /* __DEVICE_H */
