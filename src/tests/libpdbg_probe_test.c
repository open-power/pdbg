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
#include <inttypes.h>
#include <assert.h>

#include <libpdbg.h>

#include "fake.dt.h"

static void for_each_target(struct pdbg_target *parent,
			    void (*callback)(struct pdbg_target *target,
					     enum pdbg_target_status status),
			    enum pdbg_target_status status)
{
	struct pdbg_target *child;

	assert(parent);

	callback(parent, status);

	pdbg_for_each_child_target(parent, child) {
		for_each_target(child, callback, status);
	}
}

static void for_target_to_root(struct pdbg_target *target,
			       void (*callback)(struct pdbg_target *target,
						enum pdbg_target_status status),
			       enum pdbg_target_status status)
{
	assert(target);

	do {
		callback(target, status);
		target = pdbg_target_parent(NULL, target);
	} while (target != NULL);
}

static void check_status(struct pdbg_target *target,
			 enum pdbg_target_status status)
{
	struct pdbg_target *root;

	root = pdbg_target_root();
	if (target == root) {
		if (pdbg_target_status(target) != status) {
			printf("root node: status=%u, expected=%u\n",
			       pdbg_target_status(target), status);
			assert(pdbg_target_status(target) == status);
		}
		return;
	}

	assert(pdbg_target_status(target) == status);
}

static void test1(void)
{
	struct pdbg_target *root, *target;

	pdbg_targets_init(&_binary_fake_dtb_o_start);

	root = pdbg_target_root();
	assert(root);

	for_each_target(root, check_status, PDBG_TARGET_UNKNOWN);

	pdbg_target_probe_all(root);
	for_each_target(root, check_status, PDBG_TARGET_ENABLED);

	pdbg_for_each_class_target("core", target) {
		pdbg_target_release(target);
	}
	pdbg_for_each_class_target("fsi", target) {
		check_status(target, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("pib", target) {
		check_status(target, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("core", target) {
		for_each_target(target, check_status, PDBG_TARGET_RELEASED);
	}

	pdbg_for_each_class_target("pib", target) {
		pdbg_target_release(target);
	}
	pdbg_for_each_class_target("fsi", target) {
		check_status(target, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("pib", target) {
		for_each_target(target, check_status, PDBG_TARGET_RELEASED);
	}

	pdbg_for_each_class_target("fsi", target) {
		pdbg_target_release(target);
	}
	pdbg_for_each_class_target("fsi", target) {
		for_each_target(target, check_status, PDBG_TARGET_RELEASED);
	}
	check_status(root, PDBG_TARGET_ENABLED);
}

static void test2(void)
{
	struct pdbg_target *root, *target;
	enum pdbg_target_status status;

	pdbg_targets_init(&_binary_fake_dtb_o_start);

	root = pdbg_target_root();
	assert(root);

	for_each_target(root, check_status, PDBG_TARGET_UNKNOWN);

	pdbg_for_each_class_target("pib", target) {
		status = pdbg_target_probe(target);
		assert(status == PDBG_TARGET_ENABLED);

		for_target_to_root(target, check_status, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("core", target) {
		for_each_target(target, check_status, PDBG_TARGET_UNKNOWN);
	}

	pdbg_for_each_class_target("core", target) {
		status = pdbg_target_probe(target);
		assert(status == PDBG_TARGET_ENABLED);

		for_target_to_root(target, check_status, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("thread", target) {
		for_each_target(target, check_status, PDBG_TARGET_UNKNOWN);
	}

	pdbg_for_each_class_target("core", target) {
		pdbg_target_probe_all(target);
	}
	pdbg_for_each_class_target("thread", target) {
		for_each_target(target, check_status, PDBG_TARGET_ENABLED);
	}

	pdbg_for_each_class_target("core", target) {
		pdbg_target_release(target);
	}
	pdbg_for_each_class_target("fsi", target) {
		check_status(target, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("pib", target) {
		check_status(target, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("core", target) {
		check_status(target, PDBG_TARGET_RELEASED);
	}
	pdbg_for_each_class_target("thread", target) {
		check_status(target, PDBG_TARGET_RELEASED);
	}

	pdbg_for_each_class_target("fsi", target) {
		pdbg_target_release(target);
	}
	pdbg_for_each_class_target("fsi", target) {
		for_each_target(target, check_status, PDBG_TARGET_RELEASED);
	}
}

static void test3(void)
{
	struct pdbg_target *root, *target;
	enum pdbg_target_status status;

	pdbg_targets_init(&_binary_fake_dtb_o_start);

	root = pdbg_target_root();
	assert(root);

	for_each_target(root, check_status, PDBG_TARGET_UNKNOWN);

	pdbg_for_each_class_target("core", target) {
		pdbg_target_status_set(target, PDBG_TARGET_DISABLED);
	}
	pdbg_for_each_class_target("thread", target) {
		check_status(target, PDBG_TARGET_UNKNOWN);
	}

	pdbg_for_each_class_target("thread", target) {
		status = pdbg_target_probe(target);
		assert(status == PDBG_TARGET_UNKNOWN);
		check_status(target, PDBG_TARGET_UNKNOWN);
	}

	pdbg_target_probe_all(root);

	pdbg_for_each_class_target("core", target) {
		check_status(target, PDBG_TARGET_DISABLED);
	}
	pdbg_for_each_class_target("thread", target) {
		check_status(target, PDBG_TARGET_UNKNOWN);
	}

	pdbg_for_each_class_target("pib", target) {
		pdbg_target_release(target);
	}
	pdbg_for_each_class_target("fsi", target) {
		for_target_to_root(target, check_status, PDBG_TARGET_ENABLED);
	}
	pdbg_for_each_class_target("pib", target) {
		check_status(target, PDBG_TARGET_RELEASED);
	}

	pdbg_target_release(root);
	pdbg_for_each_class_target("pib", target) {
		for_target_to_root(target, check_status, PDBG_TARGET_RELEASED);
	}
	pdbg_for_each_class_target("core", target) {
		check_status(target, PDBG_TARGET_DISABLED);
	}
	pdbg_for_each_class_target("thread", target) {
		check_status(target, PDBG_TARGET_UNKNOWN);
	}
}

int main(void)
{
	int test_id = TEST_ID;

	if (test_id == 1) {
		test1();
	} else if (test_id == 2) {
		test2();
	} else if (test_id == 3) {
		test3();
	} else {
		printf("No test for TEST_ID=%d\n", test_id);
		return 1;
	}

	return 0;
}
