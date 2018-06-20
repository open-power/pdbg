/* Copyright 2017 IBM Corp.
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"

static int _getscom(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *unused)
{
	uint64_t value;

	if (pib_read(target, *addr, &value))
		return 0;

	printf("p%d:0x%" PRIx64 " = 0x%016" PRIx64 "\n", index, *addr, value);

	return 1;
}

 int getscom(uint64_t addr)
{
	return for_each_target("pib", _getscom, &addr, NULL);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getscom, getscom, (ADDRESS));

static int _putscom(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *data)
{
	if (pib_write(target, *addr, *data))
		return 0;

	return 1;
}

 int putscom(uint64_t addr, uint64_t data, uint64_t mask)
{
	/* TODO: Restore the <mask> functionality */
	return for_each_target("pib", _putscom, &addr, &data);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putscom, putscom, (ADDRESS, DATA, DEFAULT_DATA("0xffffffffffffffff")));
