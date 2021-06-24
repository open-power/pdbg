/* Copyright 2021 IBM Corp.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "target.h"
#include "hwunit.h"
#include "debug.h"
#include "libpdbg_sbe.h"

static struct chipop *pib_to_chipop(struct pdbg_target *pib)
{
	struct pdbg_target *chipop;
	uint32_t index;

	assert(pdbg_target_is_class(pib, "pib"));

	if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
		return NULL;

	index = pdbg_target_index(pib);

	pdbg_for_each_class_target("chipop", chipop) {
		if (pdbg_target_index(chipop) != index)
			continue;

		if (pdbg_target_probe(chipop) == PDBG_TARGET_ENABLED)
			return target_to_chipop(chipop);
	}

	return NULL;
}

int sbe_istep(struct pdbg_target *target, uint32_t major, uint32_t minor)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->istep) {
		PR_ERROR("istep() not implemented for the target\n");
		return -1;
	}

	return chipop->istep(chipop, major, minor);
}

int sbe_mpipl_enter(struct pdbg_target *target)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_enter) {
		PR_ERROR("mpipl_enter() not implemented for the target\n");
		return -1;
	}

	return chipop->mpipl_enter(chipop);
}

int sbe_mpipl_continue(struct pdbg_target *target)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_continue) {
		PR_ERROR("mpipl_continue() not implemented for the target\n");
		return -1;
	}

	return chipop->mpipl_continue(chipop);
}

int sbe_mpipl_get_ti_info(struct pdbg_target *target, uint8_t **data, uint32_t *data_len)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_get_ti_info) {
		PR_ERROR("mpipl_get_ti_info() not implemented for the target\n");
		return -1;
	}

	return chipop->mpipl_get_ti_info(chipop, data, data_len);
}

int sbe_dump(struct pdbg_target *target, uint8_t type, uint8_t clock, uint8_t fa_collect, uint8_t **data, uint32_t *data_len)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->dump) {
		PR_ERROR("dump() not implemented for the target\n");
		return -1;
	}

	return chipop->dump(chipop, type, clock, fa_collect, data, data_len);
}

uint32_t sbe_ffdc_get(struct pdbg_target *target, uint8_t **ffdc, uint32_t *ffdc_len)
{
	struct chipop *chipop;
	const uint8_t *data = NULL;
	uint32_t status, len = 0;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->ffdc_get) {
		PR_ERROR("ffdc_get() not implemented for the target\n");
		return -1;
	}

	status = chipop->ffdc_get(chipop, &data, &len);
	if (data && len > 0) {
		*ffdc = malloc(len);
		assert(*ffdc);
		memcpy(*ffdc, data, len);
		*ffdc_len = len;
	} else {
		*ffdc = NULL;
		*ffdc_len = 0;
	}

	return status;
}
