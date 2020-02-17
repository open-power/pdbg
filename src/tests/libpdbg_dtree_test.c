/* Copyright 2019 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libpdbg.h>

extern struct pdbg_target *get_parent(struct pdbg_target *target, bool system);

#define for_each_child(parent, target, system) \
	for (target = __pdbg_next_child_target(parent, NULL, system); \
	     target; \
	     target = __pdbg_next_child_target(parent, target, system))

static void print_tree(struct pdbg_target *target, bool system, int level)
{
	struct pdbg_target *child;
	const char *name;
	int i;

	for (i=0; i<level; i++)
		printf("   ");

	name = pdbg_target_dn_name(target);
	if (!name || name[0] == '\0')
		name = "/";
	printf("%s (%s)\n", name, pdbg_target_path(target));

	for_each_child(target, child, system)
		print_tree(child, system, level+1);
}

static void print_rtree(struct pdbg_target *target, bool system, int level)
{
	struct pdbg_target *root = pdbg_target_root();
	const char *name;
	int i;

	for (i=0; i<level; i++)
		printf("  ");

	name = pdbg_target_dn_name(target);
	if (!name || name[0] == '\0')
		name = "/";

	printf("%s\n", name);

	if (target != root) {
		target = get_parent(target, system);
		print_rtree(target, system, level+1);
	}
}

static void usage(void)
{
	fprintf(stderr, "Usage: libpdbg_dtree_test tree|rtree system|backend <path>\n");
	exit(1);
}

int main(int argc, const char **argv)
{
	struct pdbg_target *target;
	bool do_system;
	bool do_tree;

	if (argc != 4)
		usage();

	if (strcmp(argv[1], "tree") == 0) {
		do_tree = true;
	} else if (strcmp(argv[1], "rtree") == 0) {
		do_tree = false;
	} else {
		usage();
	}

	if (strcmp(argv[2], "system") == 0) {
		do_system = true;
	} else if (strcmp(argv[2], "backend") == 0) {
		do_system = false;
	} else {
		usage();
	}

	assert(pdbg_targets_init(NULL));

	target = pdbg_target_from_path(NULL, argv[3]);
	if (!target)
		exit(1);

	if (do_tree) {
		print_tree(target, do_system, 0);
	} else {
		print_rtree(target, do_system, 0);
	}

	return 0;
}
