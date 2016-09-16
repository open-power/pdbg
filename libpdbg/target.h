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

struct target;
typedef int (*target_read)(struct target *target, uint64_t addr, uint64_t *value);
typedef int (*target_write)(struct target *target, uint64_t addr, uint64_t value);
typedef void (*target_destroy)(struct target *target);

struct target {
	const char *name;
	int index;
	uint64_t base;
	target_read read;
	target_write write;
	target_destroy destroy;
	struct target *next;
	struct list_node link;
	struct list_head children;
	struct list_node class_link;
	void *priv;
	uint64_t status;
};

struct target_class {
	const char *name;
	struct list_head targets;
};

int read_target(struct target *target, uint64_t addr, uint64_t *value);
int read_next_target(struct target *target, uint64_t addr, uint64_t *value);
int write_target(struct target *target, uint64_t addr, uint64_t value);
int write_next_target(struct target *target, uint64_t addr, uint64_t value);
void target_init(struct target *target, const char *name, uint64_t base,
		 target_read read, target_write write,
		 target_destroy destroy, struct target *next);
void target_class_add(struct target_class *class, struct target *target, int index);
void target_del(struct target *target);

#endif
