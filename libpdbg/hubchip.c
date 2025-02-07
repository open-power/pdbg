/* Copyright 2016 IBM Corp.
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
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <ccan/array_size/array_size.h>
#include <unistd.h>

#include "hwunit.h"
#include "operations.h"
#include "bitutils.h"
#include "debug.h"


static struct hubchip hubchip = {
	.target = {
		.name = "Hub Chip",
		.compatible = "ibm,power12-hubchip",
		.class = "hubchip",
	},
};
DECLARE_HW_UNIT(hubchip);

static struct procmodule procmodule = {
	.target = {
		.name = "Hub Chip",
		.compatible = "ibm,power12-procmodule",
		.class = "procmodule",
	},
};
DECLARE_HW_UNIT(procmodule);

static struct sled sled = {
	.target = {
		.name = "Sled",
		.compatible = "ibm,power12-sled",
		.class = "sled",
	},
};
DECLARE_HW_UNIT(sled);

__attribute__((constructor))
static void register_hubchip(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &hubchip_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &procmodule_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sled_hw_unit);
}
