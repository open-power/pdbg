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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"
#include "path.h"

static void print_proc_reg(struct pdbg_target *target, bool is_spr, int reg, uint64_t *value, int rc)
{
	int proc_index, chip_index, thread_index;

	thread_index = pdbg_target_index(target);
	chip_index = pdbg_parent_index(target, "core");
	proc_index = pdbg_parent_index(target, "pib");
	printf("p%d:c%d:t%d: ", proc_index, chip_index, thread_index);

	if (is_spr)
		printf("%s: ", pdbg_spr_by_id(reg));
	else
		printf("gpr%02d: ", reg);

	if (rc == 0)
		printf("0x%016" PRIx64 "\n", *value);
	else if (rc == 1)
		printf("Check threadstatus - not all threads on this chiplet are quiesced\n");
	else if (rc == 2)
		printf("Thread in incorrect state\n");
	else
		printf("Error in thread access\n");
}

static int putprocreg(struct pdbg_target *target, bool is_spr, int reg, uint64_t *value)
{
	int rc;

	if (is_spr)
		rc = thread_putspr(target, reg, *value);
	else
		rc = thread_putgpr(target, reg, *value);

	return rc;
}

static int getprocreg(struct pdbg_target *target, bool is_spr, uint32_t reg, uint64_t *value)
{
	int rc;

	if (is_spr)
		rc = thread_getspr(target, reg, value);
	else
		rc = thread_getgpr(target, reg, value);

	return rc;
}

static int getreg(bool is_spr, int reg)
{
	struct pdbg_target *target;
	int count = 0;

	for_each_path_target_class("thread", target) {
		uint64_t value = 0;
		int rc;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		rc = getprocreg(target, is_spr, reg, &value);
		print_proc_reg(target, is_spr, reg, &value, rc);

		if (!rc)
			count++;
	}

	return count;
}

static int putreg(bool is_spr, int reg, uint64_t *value)
{
	struct pdbg_target *target;
	int count = 0;

	for_each_path_target_class("thread", target) {
		int rc;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		rc = putprocreg(target, is_spr, reg, value);
		print_proc_reg(target, is_spr, reg, value, rc);

		if (!rc)
			count++;
	}

	return count;
}

static int getgpr(int gpr)
{
	return getreg(false, gpr);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getgpr, getgpr, (GPR));

static int putgpr(int gpr, uint64_t data)
{
	return putreg(false, gpr, &data);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putgpr, putgpr, (GPR, DATA));

static int getspr(int spr)
{
	return getreg(true, spr);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getspr, getspr, (SPR));

static int putspr(int spr, uint64_t data)
{
	return putreg(true, spr, &data);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putspr, putspr, (SPR, DATA));
