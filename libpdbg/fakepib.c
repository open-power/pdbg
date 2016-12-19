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
#include "operations.h"

static int fake_read(struct pib *pib, uint64_t addr, uint64_t *value)
{
	*value = 0xdeadbeef;
	return 0;
}

static int fake_write(struct pib *pib, uint64_t addr, uint64_t value)
{
	return 0;
}

struct pib fake_pib = {
	.target = {
		.name =	"Fake PIB",
		.compatible = "ibm,fake-fsi",
		.class = "fsi",
	},
	.read = fake_read,
	.write = fake_write,
};
DECLARE_HW_UNIT(fake_pib);
