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

static struct sbefifo *ody_ocmb_to_sbefifo(struct ocmb *ocmb)
{
	/*uint32_t proc_index = pdbg_target_index(pdbg_target_parent("proc", &ocmb->target));
	uint32_t ocmb_index = pdbg_target_index(&ocmb->target) % 0x8;
	printf("ody_ocmb_to_sbefifo ocmb index %d proc index %d \n", ocmb_index, proc_index);

	size_t len = 0;
	uint32_t port = 0;
	struct pdbg_target *target;
	struct sbefifo *sbefifo = NULL;
	pdbg_for_each_class_target("sbefifo", target) {
		if(ocmb_index != pdbg_target_index(target)) {
			printf("ody_ocmb_to_sbefifo ocmb index %d != fifoindex %d \n", ocmb_index, pdbg_target_index(target));
			continue;
		}
		if (pdbg_target_u32_property(target, "port", &port))
		{
			char ocmb_devpath[512];
			sprintf(ocmb_devpath, "/dev/sbefifo%d%02d", proc_index+1, port);
			const char* sbefifo_devpath = (const char*)pdbg_target_property(target, "device-path", &len);
			printf("ody_ocmb_to_sbefifo computed ocmb devpath %s and sbefifo devpath %s \n", ocmb_devpath, sbefifo_devpath);
			if(strcmp(ocmb_devpath, sbefifo_devpath) == 0) {
				sbefifo = target_to_sbefifo(target);
				break;
			}
		}
	}*/
	struct pdbg_target *target;
	struct sbefifo *sbefifo = NULL;
	int my_index = 0;
	pdbg_for_each_class_target("sbefifo", target) {
		if(my_index == 9)
		{
			if (PDBG_TARGET_ENABLED == pdbg_target_probe(target))
			{
				printf(" deepa: sbefifo probed\n");
				const char* sbefifo_devpath = (const char*)pdbg_target_property(target, "device-path", NULL);
				printf("deepa: sbefifo devpath %s \n", sbefifo_devpath);
				break;
			}
			else
			{
				printf(" deepa: NOOOOOOOOOOOOO sbefifo probed\n");
			}
		}

		my_index++;
		
	}
	sbefifo = target_to_sbefifo(target);
	assert(sbefifo);

	return sbefifo;
}

static int getscom(struct sbefifo *sbefifo, struct ocmb *ocmb, uint64_t addr, uint64_t *value)
{
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t instance_id;

	instance_id = pdbg_target_index(&ocmb->target) & 0xff;

	printf("deepa instance_id: %d\n", instance_id);

	return sbefifo_hw_register_get(sctx,
				       SBEFIFO_TARGET_TYPE_OCMB,
				       instance_id,
				       addr,
				       value);
}

static int sbefifo_ocmb_getscom(struct ocmb *ocmb, uint64_t addr, uint64_t *value)
{
	struct sbefifo *sbefifo = ocmb_to_sbefifo(ocmb);
	return getscom(sbefifo, ocmb, addr, value);
}

static int putscom(struct sbefifo *sbefifo, struct ocmb *ocmb, uint64_t addr, uint64_t value)
{
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t instance_id;

	instance_id = pdbg_target_index(&ocmb->target) & 0xff;

	return sbefifo_hw_register_put(sctx,
				       SBEFIFO_TARGET_TYPE_OCMB,
				       instance_id,
				       addr,
				       value);
}

static int sbefifo_ocmb_putscom(struct ocmb *ocmb, uint64_t addr, uint64_t value)
{
	struct sbefifo *sbefifo = ocmb_to_sbefifo(ocmb);
	return putscom(sbefifo, ocmb, addr, value);
}

static int sbefifo_ody_ocmb_getscom(struct ocmb *ocmb, uint64_t addr, uint64_t *value)
{
	struct sbefifo *sbefifo = ody_ocmb_to_sbefifo(ocmb);
	return getscom(sbefifo, ocmb, addr, value);
}

static int sbefifo_ody_ocmb_putscom(struct ocmb *ocmb, uint64_t addr, uint64_t value)
{
	struct sbefifo *sbefifo = ody_ocmb_to_sbefifo(ocmb);
	return putscom(sbefifo, ocmb, addr, value);
}


static struct ocmb sbefifo_ocmb = {
	.target = {
		.name = "SBE FIFO Chip-op based OCMB",
		.compatible = "ibm,power-ocmb",
		.class = "ocmb",
	},
	.getscom = sbefifo_ocmb_getscom,
	.putscom = sbefifo_ocmb_putscom,
	.odygetscom = sbefifo_ody_ocmb_getscom,
	.odyputscom = sbefifo_ody_ocmb_putscom,
};
DECLARE_HW_UNIT(sbefifo_ocmb);

__attribute__((constructor))
static void register_ocmb(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_ocmb_hw_unit);
}
