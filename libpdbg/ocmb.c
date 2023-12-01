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

#include <fcntl.h>

static const uint16_t ODYSSEY_CHIP_ID = 0x60C0;

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

	uint32_t chipId = 0;
	pdbg_target_get_attribute(&ocmb->target, "ATTR_CHIP_ID", 4, 1, &chipId);
	if(chipId == ODYSSEY_CHIP_ID)
	{
		struct sbefifo *sbefifo = ody_ocmb_to_sbefifo(&ocmb->target);

		struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

		return sbefifo_scom_get(sctx, addr, value);
	}
	else
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
}

static int sbefifo_ocmb_putscom(struct ocmb *ocmb, uint64_t addr, uint64_t value)
{
	uint32_t chipId = 0;
	pdbg_target_get_attribute(&ocmb->target, "ATTR_CHIP_ID", 4, 1, &chipId);

	if(chipId == ODYSSEY_CHIP_ID)
	{
		struct sbefifo *sbefifo = ocmb_to_sbefifo(ocmb);
		struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

		return sbefifo_scom_put(sctx, addr, value);
	}
	else
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
}

int ocmb_probe(struct pdbg_target *target)
{
	return 0;
}

static struct ocmb sbefifo_ocmb = {
	.target = {
		.name = "SBE FIFO Chip-op based OCMB",
		.compatible = "ibm,power-ocmb",
		.class = "odyocmb",
		.probe = ocmb_probe,
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
