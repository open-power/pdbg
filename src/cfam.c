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

#include "main.h"
#include "optcmd.h"

static int _getcfam(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *unused)
{
	uint32_t value;

	if (fsi_read(target, *addr, &value))
		return 0;

	printf("p%d:0x%x = 0x%08x\n", index, (uint32_t) *addr, value);

	return 1;
}

static int getcfam(uint32_t addr)
{
	uint64_t addr64 = addr;

	return for_each_target("fsi", _getcfam, &addr64, NULL);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getcfam, getcfam, (ADDRESS32));

static int _putcfam(struct pdbg_target *target, uint32_t index, uint64_t *addr, uint64_t *data)
{
	if (fsi_write(target, *addr, *data))
		return 0;

	return 1;
}

static int putcfam(uint32_t addr, uint32_t data)
{
	uint64_t addr64 = addr, data64 = data;

	return for_each_target("fsi", _putcfam, &addr64, &data64);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putcfam, putcfam, (ADDRESS32, DATA32));
