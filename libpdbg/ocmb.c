/* Copyright 2020 IBM Corp.
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

#include <libsbefifo/libsbefifo.h>

#include "hwunit.h"

static struct sbefifo *ocmb_to_sbefifo(struct ocmb *ocmb)
{
	struct pdbg_target *pib = pdbg_target_require_parent("pib", &ocmb->target);
	struct pdbg_target *target;
	struct sbefifo *sbefifo = NULL;

	pdbg_for_each_class_target("sbefifo", target) {
		if (pdbg_target_index(target) == pdbg_target_index(pib)) {
			sbefifo = target_to_sbefifo(target);
			break;
		}
	}

	assert(sbefifo);

	return sbefifo;
}

static int sbefifo_ocmb_getscom(struct ocmb *ocmb, uint64_t addr, uint64_t *value)
{
	struct sbefifo *sbefifo = ocmb_to_sbefifo(ocmb);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t instance_id;

	instance_id = pdbg_target_index(&ocmb->target) & 0xff;

	return sbefifo_hw_register_get(sctx,
				       SBEFIFO_TARGET_TYPE_OCMB,
				       instance_id,
				       addr,
				       value);
}

static int sbefifo_ocmb_putscom(struct ocmb *ocmb, uint64_t addr, uint64_t value)
{
	struct sbefifo *sbefifo = ocmb_to_sbefifo(ocmb);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t instance_id;

	instance_id = pdbg_target_index(&ocmb->target) & 0xff;

	return sbefifo_hw_register_put(sctx,
				       SBEFIFO_TARGET_TYPE_OCMB,
				       instance_id,
				       addr,
				       value);
}

static struct ocmb sbefifo_ocmb = {
	.target = {
		.name = "OCMB",
		.compatible = "ibm,power-ocmb",
		.class = "ocmb",
	},
	.getscom = sbefifo_ocmb_getscom,
	.putscom = sbefifo_ocmb_putscom,
};
DECLARE_HW_UNIT(sbefifo_ocmb);

__attribute__((constructor))
static void register_ocmb(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_ocmb_hw_unit);
}
