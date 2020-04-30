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

#include <string.h>
#include <assert.h>

#include "hwunit.h"

#define MAX_HW_UNITS	1024
#define MAX_BACKENDS	16

static const struct hw_unit_info *g_hw_unit[MAX_BACKENDS][MAX_HW_UNITS];
static int g_hw_unit_count[MAX_BACKENDS];

void pdbg_hwunit_register(enum pdbg_backend backend, const struct hw_unit_info *hw_unit)
{
	assert(g_hw_unit_count[backend] < MAX_HW_UNITS);

	g_hw_unit[backend][g_hw_unit_count[backend]] = hw_unit;
	g_hw_unit_count[backend]++;
}

static const struct hw_unit_info *find_driver(enum pdbg_backend backend,
					      const char *compat)
{
	const struct hw_unit_info *p;
	struct pdbg_target *target;
	int i;

	for (i = 0; i < g_hw_unit_count[backend]; i++) {
		p = g_hw_unit[backend][i];
		target = p->hw_unit;

		if (!strcmp(target->compatible, compat))
			return p;
	}

	return NULL;
}

static const struct hw_unit_info *find_compatible(enum pdbg_backend backend,
						  const char *compat_list,
						  uint32_t len)
{
	const struct hw_unit_info *p;
	uint32_t i;

	i = 0;
	while (i < len) {
		p = find_driver(backend, &compat_list[i]);
		if (p)
			return p;

		i += strlen(&compat_list[i]) + 1;
	}

	return NULL;
}

const struct hw_unit_info *pdbg_hwunit_find_compatible(const char *compat_list,
						       uint32_t len)
{
	const struct hw_unit_info *p;

	p = find_compatible(pdbg_get_backend(), compat_list, len);
	if (!p)
		p = find_compatible(PDBG_DEFAULT_BACKEND, compat_list, len);

	return p;
}
