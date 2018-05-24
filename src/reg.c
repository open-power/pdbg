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

int handle_gpr(int optind, int argc, char *argv[])
{
	char *endptr;
	uint64_t gpr;

	if (optind + 1 >= argc) {
		printf("%s: command '%s' requires a GPR\n", argv[0], argv[optind]);
		return -1;
	}

	errno = 0;
	gpr = strtoull(argv[optind + 1], &endptr, 0);
	if (errno || *endptr != '\0') {
		printf("%s: command '%s' couldn't parse GPR '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
		return -1;
	}

	if (gpr > 31) {
		printf("A GPR must be between zero and 31 inclusive\n");
		return -1;
	}

	if (strcmp(argv[optind], "putgpr") == 0) {
		uint64_t data;

		if (optind + 2 >= argc) {
			printf("%s: command '%s' requires data\n", argv[0], argv[optind]);
			return -1;
		}

		errno = 0;
		data = strtoull(argv[optind + 2], &endptr, 0);
		if (errno || *endptr != '\0') {
			printf("%s: command '%s' couldn't parse data '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
			return -1;
		}

		return for_each_target("thread", putprocreg, &gpr, &data);
	}

	return for_each_target("thread", getprocreg, &gpr, NULL);
}

int handle_nia(int optind, int argc, char *argv[])
{
	uint64_t reg = REG_NIA;
	char *endptr;

	if (strcmp(argv[optind], "putnia") == 0) {
		uint64_t data;

		if (optind + 2 >= argc) {
			printf("%s: command '%s' requires data\n", argv[0], argv[optind]);
			return -1;
		}

		errno = 0;
		data = strtoull(argv[optind + 2], &endptr, 0);
		if (errno || *endptr != '\0') {
			printf("%s: command '%s' couldn't parse data '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
			return -1;
		}

		return for_each_target("thread", putprocreg, &reg, &data);
	}

	return for_each_target("thread", getprocreg, &reg, NULL);
}

int handle_spr(int optind, int argc, char *argv[])
{
	char *endptr;
	uint64_t spr;

	if (optind + 1 >= argc) {
		printf("%s: command '%s' requires a GPR\n", argv[0], argv[optind]);
		return -1;
	}

	errno = 0;
	spr = strtoull(argv[optind + 1], &endptr, 0);
	if (errno || *endptr != '\0') {
		printf("%s: command '%s' couldn't parse GPR '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
		return -1;
	}

	spr += REG_R31;

	if (strcmp(argv[optind], "putspr") == 0) {
		uint64_t data;

		if (optind + 2 >= argc) {
			printf("%s: command '%s' requires data\n", argv[0], argv[optind]);
			return -1;
		}

		errno = 0;
		data = strtoull(argv[optind + 2], &endptr, 0);
		if (errno || *endptr != '\0') {
			printf("%s: command '%s' couldn't parse data '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
			return -1;
		}

		return for_each_target("thread", putprocreg, &spr, &data);
	}

	return for_each_target("thread", getprocreg, &spr, NULL);
}

int handle_msr(int optind, int argc, char *argv[])
{
	uint64_t msr = REG_MSR;
	char *endptr;

	if (strcmp(argv[optind], "putmsr") == 0) {
		uint64_t data;

		if (optind + 2 >= argc) {
			printf("%s: command '%s' requires data\n", argv[0], argv[optind]);
			return -1;
		}

		errno = 0;
		data = strtoull(argv[optind + 2], &endptr, 0);
		if (errno || *endptr != '\0') {
			printf("%s: command '%s' couldn't parse data '%s'\n",
				argv[0], argv[optind], argv[optind + 1]);
			return -1;
		}

		return for_each_target("thread", putprocreg, &msr, &data);
	}

	return for_each_target("thread", getprocreg, &msr, NULL);
}
