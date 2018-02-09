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
#include <inttypes.h>
#include <stdio.h>

#include <target.h>
#include <operations.h>

#include "reg.h"

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

int putprocreg(struct pdbg_target *target, uint32_t index, uint64_t *reg, uint64_t *value)
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

int getprocreg(struct pdbg_target *target, uint32_t index, uint64_t *reg, uint64_t *unused)
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
