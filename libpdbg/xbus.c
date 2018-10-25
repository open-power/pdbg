/* Copyright 2018 IBM Corp.
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "target.h"
#include "bitutils.h"

/* XBus addressing is more complicated. This comes from p9_scominfo.C
 * in the ekb. */
static uint64_t xbus_translate(struct xbus *xbus, uint64_t addr)
{
	uint64_t ring = (addr >> 10) & 0xf;

	if (ring >= 0x3 && ring <= 0x5)
		addr = SETFIELD(PPC_BITMASK(50, 53), addr, 0x3 + xbus->ring_id);
	else if (ring >= 0x6 && ring <= 8)
		addr = SETFIELD(PPC_BITMASK(50, 53), addr, 0x6 + xbus->ring_id);

	return addr;
}

static int xbus_probe(struct pdbg_target *target)
{
	struct xbus *xbus = target_to_xbus(target);

	if (pdbg_target_u32_property(&xbus->target, "ring-id", &xbus->ring_id)) {
		printf("Unknown ring-id on %s@%d\n", pdbg_target_name(&xbus->target),
		       pdbg_target_index(&xbus->target));
		return -1;
	}

	return 0;
}

struct xbus p9_xbus = {
        .target = {
                .name = "POWER9 XBus",
                .compatible = "ibm,xbus",
                .class = "xbus",
		.probe = xbus_probe,
		.translate = translate_cast(xbus_translate),
        },
};
DECLARE_HW_UNIT(p9_xbus);
