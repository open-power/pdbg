/* Copyright 2018 IBM Corp.
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
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <libpdbg.h>

#include "fake.dt.h"

static int count_target(struct pdbg_target *parent, const char *classname)
{
	struct pdbg_target *target;
	int n = 0;

	pdbg_for_each_target(classname, parent, target) {
		n++;
	}

	return n;
}

static int count_class_target(const char *classname)
{
	struct pdbg_target *target;
	int n = 0;

	pdbg_for_each_class_target(classname, target) {
		n++;
	}

	return n;
}

static int count_child_target(struct pdbg_target *parent)
{
	struct pdbg_target *child;
	int n = 0;

	pdbg_for_each_child_target(parent, child) {
		n++;
	}

	return n;
}

int main(void)
{
	struct pdbg_target *root, *target, *parent, *parent2;
	const char *name;
	int count, i;

	pdbg_set_backend(PDBG_BACKEND_FAKE, NULL);
	assert(pdbg_targets_init(NULL));

	root = pdbg_target_root();
	assert(root);

	count = count_class_target("fsi");
	assert(count == 8);

	count = count_class_target("pib");
	assert(count == 8);

	count = count_class_target("core");
	assert(count == 32);

	count = count_class_target("thread");
	assert(count == 64);

	pdbg_for_each_child_target(root, parent) {
		name = pdbg_target_dn_name(parent);
		assert(strncmp(name, "proc", 4) == 0);

		pdbg_for_each_target("pib", parent, target) {
			name = pdbg_target_class_name(target);
			assert(!strcmp(name, "pib"));
		}

		pdbg_for_each_target("fsi", parent, target) {
			name = pdbg_target_class_name(target);
			assert(!strcmp(name, "fsi"));
		}
	};

	pdbg_for_each_class_target("fsi", target) {
		parent = pdbg_target_parent(NULL, target);
		assert(parent);

		name = pdbg_target_dn_name(parent);
		assert(strncmp(name, "proc", 4) == 0);

		parent = pdbg_target_parent("fsi", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("pib", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("core", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("thread", target);
		assert(parent == NULL);

		count = count_child_target(target);
		assert(count == 0);

		count = count_target(target, "fsi");
		assert(count == 1);

		count = count_target(target, "pib");
		assert(count == 0);

		count = count_target(target, "core");
		assert(count == 0);

		count = count_target(target, "thread");
		assert(count == 0);

		name = pdbg_target_name(target);
		assert(!strcmp(name, "Fake FSI"));

		name = pdbg_target_class_name(target);
		assert(!strcmp(name, "fsi"));

		name = pdbg_target_dn_name(target);
		assert(!strncmp(name, "fsi", 3));
	}

	pdbg_for_each_class_target("pib", target) {
		parent = pdbg_target_parent(NULL, target);
		assert(parent);

		name = pdbg_target_dn_name(parent);
		assert(strncmp(name, "proc", 4) == 0);

		parent = pdbg_target_parent("fsi", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("pib", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("core", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("thread", target);
		assert(parent == NULL);

		count = count_child_target(target);
		assert(count == 4);

		count = count_target(target, "fsi");
		assert(count == 0);

		count = count_target(target, "pib");
		assert(count == 1);

		count = count_target(target, "core");
		assert(count == 4);

		count = count_target(target, "thread");
		assert(count == 8);

		name = pdbg_target_name(target);
		assert(!strcmp(name, "Fake PIB"));

		name = pdbg_target_class_name(target);
		assert(!strcmp(name, "pib"));

		name = pdbg_target_dn_name(target);
		assert(!strncmp(name, "pib", 3));
	}

	pdbg_for_each_class_target("core", target) {
		uint64_t addr, size;
		uint32_t index;

		parent = pdbg_target_parent("fsi", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("pib", target);
		assert(parent);

		parent2 = pdbg_target_require_parent("pib", target);
		assert(parent == parent2);

		parent = pdbg_target_parent("core", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("thread", target);
		assert(parent == NULL);

		count = count_child_target(target);
		assert(count == 2);

		count = count_target(target, "fsi");
		assert(count == 0);

		count = count_target(target, "pib");
		assert(count == 0);

		count = count_target(target, "core");
		assert(count == 1);

		count = count_target(target, "thread");
		assert(count == 2);

		name = pdbg_target_name(target);
		assert(!strcmp(name, "Fake Core"));

		name = pdbg_target_class_name(target);
		assert(!strcmp(name, "core"));

		name = pdbg_target_dn_name(target);
		assert(!strncmp(name, "core", 4));

		index = pdbg_target_index(target);
		addr = pdbg_target_address(target, &size);
		assert(size == 0);
		assert(addr == 0x10000 + (index + 1)*0x10);
	}

	pdbg_for_each_class_target("thread", target) {
		parent = pdbg_target_parent("fsi", target);
		assert(parent == NULL);

		parent = pdbg_target_parent("pib", target);
		assert(parent);

		parent2 = pdbg_target_require_parent("pib", target);
		assert(parent == parent2);

		parent = pdbg_target_parent("core", target);
		assert(parent);

		parent2 = pdbg_target_require_parent("core", target);
		assert(parent == parent2);

		parent = pdbg_target_parent("thread", target);
		assert(parent == NULL);

		count = count_child_target(target);
		assert(count == 0);

		count = count_target(target, "fsi");
		assert(count == 0);

		count = count_target(target, "pib");
		assert(count == 0);

		count = count_target(target, "core");
		assert(count == 0);

		count = count_target(target, "thread");
		assert(count == 1);

		name = pdbg_target_name(target);
		assert(!strcmp(name, "Fake Thread"));

		name = pdbg_target_class_name(target);
		assert(!strcmp(name, "thread"));

		name = pdbg_target_dn_name(target);
		assert(!strncmp(name, "thread", 6));
	}

	return 0;
}
