/* Copyright 2020 IBM Corp.
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
#include <assert.h>

#include <libpdbg.h>

static int print_target(struct pdbg_target *target, void *priv)
{
	const char *path;
	int *level = (int *)priv;
	int i;

	path = pdbg_target_path(target);
	assert(path);

	for (i=0; i<*level; i++)
		printf("  ");

	printf("%s\n", path);
	return 0;
}

int main(int argc, const char **argv)
{
	struct pdbg_target *parent = NULL;
	int level = 0;

	assert(pdbg_targets_init(NULL));

	if (argc == 2)
		parent = pdbg_target_from_path(NULL, argv[1]);

	return pdbg_target_traverse(parent, print_target, &level);
}

