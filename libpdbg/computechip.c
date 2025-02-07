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


static struct computechip computechip = {
	.target = {
		.name = "Compute Chip",
		.compatible = "ibm,power12-computechip",
		.class = "computechip",
	},
};
DECLARE_HW_UNIT(computechip);

__attribute__((constructor))
static void register_computechip(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &computechip_hw_unit);
}
