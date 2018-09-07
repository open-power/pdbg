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

#include <libpdbg.h>

#include "main.h"
#include "optcmd.h"

#define REG_MEM -3
#define REG_MSR -2
#define REG_NIA -1
#define REG_R31 31

static void print_proc_reg(struct pdbg_target *target, uint64_t reg, uint64_t value, int rc)
{
	int proc_index, chip_index, thread_index;

	thread_index = pdbg_target_index(target);
	chip_index = pdbg_parent_index(target, "core");
	proc_index = pdbg_parent_index(target, "pib");
	printf("p%d:c%d:t%d:", proc_index, chip_index, thread_index);

	if (reg == REG_MSR)
		printf("msr: ");
	else if (reg == REG_NIA)
		printf("nia: ");
	else if (reg > REG_R31)
		printf("spr%03" PRIu64 ": ", reg - REG_R31);
	else if (reg >= 0 && reg <= 31)
		printf("gpr%02" PRIu64 ": ", reg);

	if (rc == 1) {
		printf("Check threadstatus - not all threads on this chiplet are quiesced\n");
	} else if (rc == 2)
		printf("Thread in incorrect state\n");
	else
		printf("0x%016" PRIx64 "\n", value);
}

static int putprocreg(struct pdbg_target *target, uint32_t index, uint64_t *reg, uint64_t *value)
{
	int rc;

	if (*reg == REG_MSR)
		rc = ram_putmsr(target, *value);
	else if (*reg == REG_NIA)
		rc = ram_putnia(target, *value);
	else if (*reg > REG_R31)
		rc = ram_putspr(target, *reg - REG_R31, *value);
	else if (*reg >= 0 && *reg <= 31)
		rc = ram_putgpr(target, *reg, *value);

	print_proc_reg(target, *reg, *value, rc);

	return 0;
}

static int getprocreg(struct pdbg_target *target, uint32_t index, uint64_t *reg, uint64_t *unused)
{
	int rc;
	uint64_t value;

	if (*reg == REG_MSR)
		rc = ram_getmsr(target, &value);
	else if (*reg == REG_NIA)
		rc = ram_getnia(target, &value);
	else if (*reg > REG_R31)
		rc = ram_getspr(target, *reg - REG_R31, &value);
	else if (*reg >= 0 && *reg <= 31)
		rc = ram_getgpr(target, *reg, &value);

	print_proc_reg(target, *reg, value, rc);

	return !rc;
}

static int getgpr(int gpr)
{
	uint64_t reg = gpr;
	return for_each_target("thread", getprocreg, &reg, NULL);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getgpr, getgpr, (GPR));

static int putgpr(int gpr, uint64_t data)
{
	uint64_t reg = gpr;
	return for_each_target("thread", putprocreg, &reg, &data);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putgpr, putgpr, (GPR, DATA));

static int getnia(void)
{
	uint64_t reg = REG_NIA;
	return for_each_target("thread", getprocreg, &reg, NULL);
}
OPTCMD_DEFINE_CMD(getnia, getnia);

static int putnia(uint64_t nia)
{
	uint64_t reg = REG_NIA;
	return for_each_target("thread", putprocreg, &reg, &nia);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putnia, putnia, (DATA));

static int getspr(int spr)
{
	uint64_t reg = spr + REG_R31;
	return for_each_target("thread", getprocreg, &reg, NULL);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(getspr, getspr, (SPR));

static int putspr(int spr, uint64_t data)
{
	uint64_t reg = spr + REG_R31;
	return for_each_target("thread", putprocreg, &reg, &data);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putspr, putspr, (SPR, DATA));

static int getmsr(void)
{
	uint64_t reg = REG_MSR;
	return for_each_target("thread", getprocreg, &reg, NULL);
}
OPTCMD_DEFINE_CMD(getmsr, getmsr);

static int putmsr(uint64_t data)
{
	uint64_t reg = REG_MSR;
	return for_each_target("thread", putprocreg, &reg, &data);
}
OPTCMD_DEFINE_CMD_WITH_ARGS(putmsr, putmsr, (DATA));
