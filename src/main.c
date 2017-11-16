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
#define _GNU_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <inttypes.h>

#include <backend.h>
#include <operations.h>
#include <target.h>
#include <device.h>

#include <config.h>

#include "bitutils.h"

#undef PR_DEBUG
#define PR_DEBUG(...)

#define HTM_DUMP_BASENAME "htm.dump"

enum command { GETCFAM = 1, PUTCFAM, GETSCOM, PUTSCOM,	\
	       GETMEM, PUTMEM, GETGPR, GETNIA, GETSPR,	\
	       GETMSR, PUTGPR, PUTNIA, PUTSPR, PUTMSR,	\
	       STOP, START, THREADSTATUS, STEP, PROBE,	\
	       GETVMEM, SRESET, HTM_STOP, HTM_ANALYSE,  \
	       HTM_START, HTM_DUMP, HTM_RESET, HTM_GO,  \
	       HTM_TRACE, HTM_STATUS };

#define MAX_CMD_ARGS 3
enum command cmd = 0;
static int cmd_arg_count = 0;
static int cmd_min_arg_count = 0;
static int cmd_max_arg_count = 0;

/* At the moment all commands only take some kind of number */
static uint64_t cmd_args[MAX_CMD_ARGS];

enum backend { FSI, I2C, KERNEL, FAKE, HOST };
static enum backend backend = KERNEL;

static char const *device_node;
static int i2c_addr = 0x50;

#define MAX_PROCESSORS 16
#define MAX_CHIPS 24
#define MAX_THREADS THREADS_PER_CORE

static int **processorsel[MAX_PROCESSORS];
static int *chipsel[MAX_PROCESSORS][MAX_CHIPS];
static int threadsel[MAX_PROCESSORS][MAX_CHIPS][MAX_THREADS];

/* Convenience functions */
#define for_each_thread(x) while(0)
#define for_each_chiplet(x) while(0)
#define for_each_processor(x) while(0)
#define for_each_cfam(x) while(0)

static void print_usage(char *pname)
{
	printf("Usage: %s [options] command ...\n\n", pname);
	printf(" Options:\n");
	printf("\t-p, --processor=processor-id\n");
	printf("\t-c, --chip=chiplet-id\n");
	printf("\t-t, --thread=thread\n");
	printf("\t-a, --all\n");
	printf("\t\tRun command on all possible processors/chips/threads (default)\n");
	printf("\t-b, --backend=backend\n");
	printf("\t\tfsi:\tAn experimental backend that uses\n");
	printf("\t\t\tbit-banging to access the host processor\n");
	printf("\t\t\tvia the FSI bus.\n");
	printf("\t\ti2c:\tThe P8 only backend which goes via I2C.\n");
	printf("\t\thost:\tUse the debugfs xscom nodes.\n");
	printf("\t\tkernel:\tThe default backend which goes the kernel FSI driver.\n");
	printf("\t-d, --device=backend device\n");
	printf("\t\tFor I2C the device node used by the backend to access the bus.\n");
	printf("\t\tFor FSI the system board type, one of p8 or p9w\n");
	printf("\t\tDefaults to /dev/i2c4 for I2C\n");
	printf("\t-s, --slave-address=backend device address\n");
	printf("\t\tDevice slave address to use for the backend. Not used by FSI\n");
	printf("\t\tand defaults to 0x50 for I2C\n");
	printf("\t-V, --version\n");
	printf("\t-h, --help\n");
	printf("\n");
	printf(" Commands:\n");
	printf("\tgetcfam <address>\n");
	printf("\tputcfam <address> <value> [<mask>]\n");
	printf("\tgetscom <address>\n");
	printf("\tputscom <address> <value> [<mask>]\n");
	printf("\tgetmem <address> <count>\n");
	printf("\tputmem <address>\n");
	printf("\tgetvmem <virtual address>\n");
	printf("\tgetgpr <gpr>\n");
	printf("\tputgpr <gpr> <value>\n");
	printf("\tgetnia\n");
	printf("\tputnia <value>\n");
	printf("\tgetspr <spr>\n");
	printf("\tputspr <spr> <value>\n");
	printf("\tstart\n");
	printf("\tstep <count>\n");
	printf("\tstop\n");
	printf("\tthreadstatus\n");
	printf("\tprobe\n");
	printf("\thtm_start\n");
	printf("\thtm_stop\n");
	printf("\thtm_status\n");
	printf("\thtm_reset\n");
	printf("\thtm_dump\n");
	printf("\thtm_trace\n");
	printf("\thtm_analyse\n");
}

enum command parse_cmd(char *optarg)
{
	cmd_max_arg_count = 0;

	if (strcmp(optarg, "getcfam") == 0) {
		cmd = GETCFAM;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "getscom") == 0) {
		cmd = GETSCOM;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "putcfam") == 0) {
		cmd = PUTCFAM;
		cmd_min_arg_count = 2;
		cmd_max_arg_count = 3;

		/* No mask by default */
		cmd_args[2] = -1ULL;
	} else if (strcmp(optarg, "putscom") == 0) {
		cmd = PUTSCOM;
		cmd_min_arg_count = 2;
		cmd_max_arg_count = 3;

		/* No mask by default */
		cmd_args[2] = -1ULL;
	} else if (strcmp(optarg, "getmem") == 0) {
		cmd = GETMEM;
		cmd_min_arg_count = 2;
	} else if (strcmp(optarg, "putmem") == 0) {
		cmd = PUTMEM;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "getgpr") == 0) {
		cmd = GETGPR;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "putgpr") == 0) {
		cmd = PUTGPR;
		cmd_min_arg_count = 2;
	} else if (strcmp(optarg, "getnia") == 0) {
		cmd = GETNIA;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "putnia") == 0) {
		cmd = PUTNIA;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "getspr") == 0) {
		cmd = GETSPR;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "putspr") == 0) {
		cmd = PUTSPR;
		cmd_min_arg_count = 2;
	} else if (strcmp(optarg, "getmsr") == 0) {
		cmd = GETMSR;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "putmsr") == 0) {
		cmd = PUTMSR;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "getvmem") == 0) {
		cmd = GETVMEM;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "start") == 0) {
		cmd = START;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "step") == 0) {
		cmd = STEP;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "stop") == 0) {
		cmd = STOP;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "sreset") == 0) {
		cmd = SRESET;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "threadstatus") == 0) {
		cmd = THREADSTATUS;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "probe") == 0) {
		cmd = PROBE;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_start") == 0) {
		cmd = HTM_START;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_go") == 0) {
		cmd = HTM_START;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_stop") == 0) {
		cmd = HTM_STOP;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_status") == 0) {
		cmd = HTM_STATUS;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_reset") == 0) {
		cmd = HTM_RESET;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_dump") == 0) {
		cmd = HTM_DUMP;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_trace") == 0) {
		cmd = HTM_TRACE;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "htm_analyse") == 0) {
		cmd = HTM_ANALYSE;
		cmd_min_arg_count = 0;
	}

	if (cmd_min_arg_count && !cmd_max_arg_count)
		cmd_max_arg_count = cmd_min_arg_count;

	return cmd;
}

static bool parse_options(int argc, char *argv[])
{
	int c, oidx = 0, cmd_arg_idx = 0;
	bool opt_error = true;
	static int current_processor = INT_MAX, current_chip = INT_MAX, current_thread = INT_MAX;
	struct option long_opts[] = {
		{"all",			no_argument,		NULL,	'a'},
		{"processor",		required_argument,	NULL,	'p'},
		{"chip",		required_argument,	NULL,	'c'},
		{"thread",		required_argument,	NULL,	't'},
		{"backend",		required_argument,	NULL,	'b'},
		{"device",		required_argument,	NULL,	'd'},
		{"slave-address",	required_argument,	NULL,	's'},
		{"version",		no_argument,		NULL,	'V'},
		{"help",		no_argument,		NULL,	'h'},
	};

	do {
		c = getopt_long(argc, argv, "-p:c:t:b:d:s:haV", long_opts, &oidx);
		switch(c) {
		case 1:
			/* Positional argument */
			if (!cmd)
				opt_error = !parse_cmd(optarg);
			else if (cmd_arg_idx >= MAX_CMD_ARGS ||
				 (cmd && cmd_arg_idx >= cmd_max_arg_count))
				opt_error = true;
			else {
				errno = 0;
				cmd_args[cmd_arg_idx++] = strtoull(optarg, NULL, 0);
				opt_error = errno;
			}
			break;

		case 'a':
			opt_error = false;
			for (current_processor = 0; current_processor < MAX_PROCESSORS; current_processor++) {
				processorsel[current_processor] = &chipsel[current_processor][0];
				for (current_chip = 0; current_chip < MAX_CHIPS; current_chip++) {
					chipsel[current_processor][current_chip] = &threadsel[current_processor][current_chip][0];
					for (current_thread = 0; current_thread < MAX_THREADS; current_thread++)
						threadsel[current_processor][current_chip][current_thread] = 1;
				}
			}
			break;

		case 'p':
			errno = 0;
			current_processor = strtoul(optarg, NULL, 0);
			if (current_processor >= MAX_PROCESSORS)
				errno = -1;
			else
				processorsel[current_processor] = &chipsel[current_processor][0];
			opt_error = errno;
			break;

		case 'c':
			errno = 0;
			current_chip = strtoul(optarg, NULL, 0);
			if (current_chip >= MAX_CHIPS)
				errno = -1;
			else
				chipsel[current_processor][current_chip] = &threadsel[current_processor][current_chip][0];
			opt_error = errno;
			break;

		case 't':
			errno = 0;
			current_thread = strtoul(optarg, NULL, 0);
			if (current_thread >= MAX_THREADS)
				errno = -1;
			else
				threadsel[current_processor][current_chip][current_thread] = 1;
			opt_error = errno;
			break;

		case 'b':
			opt_error = false;
			if (strcmp(optarg, "fsi") == 0) {
				backend = FSI;
				device_node = "p9w";
			} else if (strcmp(optarg, "i2c") == 0) {
				backend = I2C;
				device_node = "/dev/i2c4";
			} else if (strcmp(optarg, "kernel") == 0) {
				backend = KERNEL;
				/* TODO: use device node to point at a slave
				 * other than the first? */
			} else if (strcmp(optarg, "fake") == 0) {
				backend = FAKE;
			} else if (strcmp(optarg, "host") == 0) {
				backend = HOST;
			} else
				opt_error = true;
			break;

		case 'd':
			opt_error = false;
			device_node = optarg;
			break;

		case 's':
			errno = 0;
			i2c_addr = strtoull(optarg, NULL, 0);
			opt_error = errno;
			break;

		case 'V':
			errno = 0;
			printf("%s (commit %s)\n", PACKAGE_STRING, GIT_SHA1);
			exit(1);
			break;

		case 'h':
			opt_error = true;
			break;
		}
	} while (c != EOF && !opt_error);

	opt_error |= cmd_arg_idx < cmd_min_arg_count;
	if (opt_error)
		print_usage(argv[0]);

	cmd_arg_count = cmd_arg_idx;

	return opt_error;
}

/* Returns the sum of return codes. This can be used to count how many targets the callback was run on. */
static int for_each_child_target(char *class, struct target *parent,
				 int (*cb)(struct target *, uint32_t, uint64_t *, uint64_t *),
				 uint64_t *arg1, uint64_t *arg2)
{
	int rc = 0;
	struct target *target;
	uint32_t index;
	struct dt_node *dn;

	for_each_class_target(class, target) {
		struct dt_property *p;

		dn = target->dn;
		if (parent && dn->parent != parent->dn)
			continue;

		/* Search up the tree for an index */
		for (index = dn->target->index; index == -1; dn = dn->parent);
		assert(index != -1);

		p = dt_find_property(dn, "status");
		if (p && (!strcmp(p->prop, "disabled") || !strcmp(p->prop, "hidden")))
			continue;

		rc += cb(target, index, arg1, arg2);
	}

	return rc;
}

static int for_each_target(char *class, int (*cb)(struct target *, uint32_t, uint64_t *, uint64_t *), uint64_t *arg1, uint64_t *arg2)
{
	return for_each_child_target(class, NULL, cb, arg1, arg2);
}

static int getcfam(struct target *target, uint32_t index, uint64_t *addr, uint64_t *unused)
{
	uint32_t value;

	if (fsi_read(target, *addr, &value))
		return 0;

	printf("p%d:0x%x = 0x%08x\n", index, (uint32_t) *addr, value);

	return 1;
}

static int putcfam(struct target *target, uint32_t index, uint64_t *addr, uint64_t *data)
{
	if (fsi_write(target, *addr, *data))
		return 0;

	return 1;
}

static int getscom(struct target *target, uint32_t index, uint64_t *addr, uint64_t *unused)
{
	uint64_t value;

	if (pib_read(target, *addr, &value))
		return 0;

	printf("p%d:0x%" PRIx64 " = 0x%016" PRIx64 "\n", index, *addr, value);

	return 1;
}

static int putscom(struct target *target, uint32_t index, uint64_t *addr, uint64_t *data)
{
	if (pib_write(target, *addr, *data))
		return 0;

	return 1;
}

static int print_thread_status(struct target *thread_target, uint32_t index, uint64_t *status, uint64_t *unused1)
{
	struct thread *thread = target_to_thread(thread_target);

	*status = SETFIELD(0xf << (index * 4), *status, thread_status(thread) & 0xf);
	return 1;
}

static int print_chiplet_thread_status(struct target *chiplet_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	uint64_t status = -1UL;
	int i, rc;

	printf("c%02d:", index);
	rc = for_each_child_target("thread", chiplet_target, print_thread_status, &status, NULL);
	for (i = 0; i < 8; i++)
		switch ((status >> (i * 4)) & 0xf) {
		case THREAD_STATUS_ACTIVE:
			printf(" A");
			break;

		case THREAD_STATUS_DOZE:
		case THREAD_STATUS_QUIESCE | THREAD_STATUS_DOZE:
			printf(" D");
			break;

		case THREAD_STATUS_NAP:
		case THREAD_STATUS_QUIESCE | THREAD_STATUS_NAP:
			printf(" N");
			break;

		case THREAD_STATUS_SLEEP:
		case THREAD_STATUS_QUIESCE | THREAD_STATUS_SLEEP:
			printf(" S");
			break;

		case THREAD_STATUS_ACTIVE | THREAD_STATUS_QUIESCE:
			printf(" Q");
			break;

		case 0xf:
			printf("  ");
			break;

		default:
			printf(" U");
			break;
		}
	printf("\n");

	return rc;
}

static int print_proc_thread_status(struct target *pib_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	printf("\np%01dt: 0 1 2 3 4 5 6 7\n", index);
	return for_each_child_target("chiplet", pib_target, print_chiplet_thread_status, NULL, NULL);
};

#define REG_MEM -3
#define REG_MSR -2
#define REG_NIA -1
#define REG_R31 31
static void print_proc_reg(struct thread *thread, uint64_t reg, uint64_t value, int rc)
{
	int proc_index, chip_index, thread_index;

	thread_index = thread->target.index;
	chip_index = thread->target.dn->parent->target->index;
	proc_index = thread->target.dn->parent->parent->target->index;
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

static int putprocreg(struct target *thread_target, uint32_t index, uint64_t *reg, uint64_t *value)
{
	struct thread *thread = target_to_thread(thread_target);
	int rc;

	if (*reg == REG_MSR)
		rc = ram_putmsr(thread, *value);
	else if (*reg == REG_NIA)
		rc = ram_putnia(thread, *value);
	else if (*reg > REG_R31)
		rc = ram_putspr(thread, *reg - REG_R31, *value);
	else if (*reg >= 0 && *reg <= 31)
		rc = ram_putgpr(thread, *reg, *value);

	print_proc_reg(thread, *reg, *value, rc);

	return 0;
}

static int getprocreg(struct target *thread_target, uint32_t index, uint64_t *reg, uint64_t *unused)
{
	struct thread *thread = target_to_thread(thread_target);
	int rc;
	uint64_t value;

	if (*reg == REG_MSR)
		rc = ram_getmsr(thread, &value);
	else if (*reg == REG_NIA)
		rc = ram_getnia(thread, &value);
	else if (*reg > REG_R31)
		rc = ram_getspr(thread, *reg - REG_R31, &value);
	else if (*reg >= 0 && *reg <= 31)
		rc = ram_getgpr(thread, *reg, &value);

	print_proc_reg(thread, *reg, value, rc);

	return !rc;
}

#define PUTMEM_BUF_SIZE 1024
static int putmem(uint64_t addr)
{
        uint8_t *buf;
        int read_size, rc = 0;
        struct target *adu_target;

	for_each_class_target("adu", adu_target)
		break;

        buf = malloc(PUTMEM_BUF_SIZE);
        assert(buf);
	do {
                read_size = read(STDIN_FILENO, buf, PUTMEM_BUF_SIZE);
                if (adu_putmem(adu_target, addr, buf, read_size)) {
                        rc = 0;
                        PR_ERROR("Unable to write memory.\n");
                        break;
                }
                rc += read_size;
        } while (read_size > 0);

        free(buf);
        return rc;
}

static int start_thread(struct target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	return ram_start_thread(thread_target) ? 0 : 1;
}

static int step_thread(struct target *thread_target, uint32_t index, uint64_t *count, uint64_t *unused1)
{
	return ram_step_thread(thread_target, *count) ? 0 : 1;
}

static int stop_thread(struct target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	return ram_stop_thread(thread_target) ? 0 : 1;
}

static int sreset_thread(struct target *thread_target, uint32_t index, uint64_t *unused, uint64_t *unused1)
{
	return ram_sreset_thread(thread_target) ? 0 : 1;
}

static void enable_dn(struct dt_node *dn)
{
	struct dt_property *p;

	PR_DEBUG("Enabling %s\n", dn->name);
	p = dt_find_property(dn, "status");

	if (!p)
		/* Default assumption enabled */
		return;

	/* We only override a status of "hidden" */
	if (strcmp(p->prop, "hidden"))
		return;

	dt_del_property(dn, p);
}

static void disable_dn(struct dt_node *dn)
{
	struct dt_property *p;

	PR_DEBUG("Disabling %s\n", dn->name);
	p = dt_find_property(dn, "status");
	if (p)
		/* We don't override hard-coded device tree
		 * status. This is needed to avoid disabling that
		 * backend. */
		return;

	dt_add_property_string(dn, "status", "disabled");
}

static char *get_htm_dump_filename(void)
{
	char *filename;
	int i;

	filename = strdup(HTM_DUMP_BASENAME);
	if (!filename)
		return NULL;

	i = 0;
	while (access(filename, F_OK) == 0) {
		free(filename);
		if (asprintf(&filename, "%s.%d", HTM_DUMP_BASENAME, i) == -1)
			return NULL;
		i++;
	}

	return filename;
}

static int run_htm_start(void)
{
	struct target *target;
	int rc = 0;

	for_each_class_target("htm", target) {
		printf("Starting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_start(target) != 1)
			printf("Couldn't start HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}

	return rc;
}

static int run_htm_stop(void)
{
	struct target *target;
	int rc = 0;

	for_each_class_target("htm", target) {
		printf("Stopping HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_stop(target) != 1)
			printf("Couldn't stop HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}

	return rc;
}

static int run_htm_status(void)
{
	struct target *target;
	int rc = 0;

	for_each_class_target("htm", target) {
		printf("HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_status(target) != 1)
			printf("Couldn't get HTM@%d#%d status\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
		printf("\n\n");
	}

	return rc;
}

static int run_htm_reset(void)
{
	uint64_t old_base = 0, base, size;
	struct target *target;
	int rc = 0;

	for_each_class_target("htm", target) {
		printf("Resetting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_reset(target, &base, &size) != 1)
			printf("Couldn't reset HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		if (old_base != base) {
			printf("The kernel has initialised HTM memory at:\n");
			printf("base: 0x%016" PRIx64 " for 0x%016" PRIx64 " size\n",
				base, size);
			printf("In case of system crash/xstop use the following to dump the trace on the BMC:\n");
			printf("./pdbg getmem 0x%016" PRIx64 " 0x%016" PRIx64 " > htm.dump\n",
					base, size);
		}
		rc++;
	}

	return rc;
}

static int run_htm_dump(void)
{
	struct target *target;
	char *filename;
	int rc = 0;

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	/* size = 0 will dump everything */
	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	for_each_class_target("htm", target) {
		printf("Dumping HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_dump(target, 0, filename) == 1)
			printf("Couldn't dump HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}
	free(filename);

	return rc;
}

static int run_htm_trace(void)
{
	uint64_t old_base = 0, base, size;
	struct target *target;
	int rc = 0;

	for_each_class_target("htm", target) {
		/*
		 * Don't mind if stop fails, it will fail if it wasn't
		 * running, if anything bad is happening reset will fail
		 */
		htm_stop(target);
		printf("Resetting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_reset(target, &base, &size) != 1)
			printf("Couldn't reset HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		if (old_base != base) {
			printf("The kernel has initialised HTM memory at:\n");
			printf("base: 0x%016" PRIx64 " for 0x%016" PRIx64 " size\n",
					base, size);
			printf("./pdbg getmem 0x%016" PRIx64 " 0x%016" PRIx64 " > htm.dump\n\n",
					base, size);
		}
		old_base = base;
	}

	for_each_class_target("htm", target) {
		printf("Starting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_start(target) != 1)
			printf("Couldn't start HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}

	return rc;
}

static int run_htm_analyse(void)
{
	struct target *target;
	char *filename;
	int rc = 0;

	for_each_class_target("htm", target)
		htm_stop(target);

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	for_each_class_target("htm", target) {
		printf("Dumping HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_dump(target, 0, filename) != 1)
			printf("Couldn't dump HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}
	free(filename);

	return rc;
}

/* TODO: It would be nice to have a more dynamic way of doing this */
extern unsigned char _binary_p8_i2c_dtb_o_start;
extern unsigned char _binary_p8_i2c_dtb_o_end;
extern unsigned char _binary_p8_fsi_dtb_o_start;
extern unsigned char _binary_p8_fsi_dtb_o_end;
extern unsigned char _binary_p9w_fsi_dtb_o_start;
extern unsigned char _binary_p9w_fsi_dtb_o_end;
extern unsigned char _binary_p9r_fsi_dtb_o_start;
extern unsigned char _binary_p9r_fsi_dtb_o_end;
extern unsigned char _binary_p9z_fsi_dtb_o_start;
extern unsigned char _binary_p9z_fsi_dtb_o_end;
extern unsigned char _binary_p9_kernel_dtb_o_start;
extern unsigned char _binary_p9_kernel_dtb_o_end;
extern unsigned char _binary_fake_dtb_o_start;
extern unsigned char _binary_fake_dtb_o_end;
extern unsigned char _binary_p8_host_dtb_o_start;
extern unsigned char _binary_p8_host_dtb_o_end;
extern unsigned char _binary_p9_host_dtb_o_start;
extern unsigned char _binary_p9_host_dtb_o_end;
static int target_select(void)
{
	struct target *fsi, *pib, *chip, *thread;

	switch (backend) {
	case I2C:
		targets_init(&_binary_p8_i2c_dtb_o_start);
		break;

	case FSI:
		if (device_node == NULL) {
			PR_ERROR("FSI backend requires a device type\n");
			return -1;
		}
		if (!strcmp(device_node, "p8"))
			targets_init(&_binary_p8_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9w") || !strcmp(device_node, "witherspoon"))
			targets_init(&_binary_p9w_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9r") || !strcmp(device_node, "romulus"))
			targets_init(&_binary_p9r_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9z") || !strcmp(device_node, "zaius"))
			targets_init(&_binary_p9z_fsi_dtb_o_start);
		else {
			PR_ERROR("Invalid device type specified\n");
			return -1;
		}
		break;

	case KERNEL:
		targets_init(&_binary_p9_kernel_dtb_o_start);
		break;

	case FAKE:
		targets_init(&_binary_fake_dtb_o_start);
		break;

	case HOST:
		if (device_node == NULL) {
			PR_ERROR("Host backend requires a device type\n");
			return -1;
		}
		if (!strcmp(device_node, "p8"))
			targets_init(&_binary_p8_host_dtb_o_start);
		else if (!strcmp(device_node, "p9"))
			targets_init(&_binary_p9_host_dtb_o_start);
		else {
			PR_ERROR("Unsupported device type for host backend\n");
			return -1;
		}
		break;

	default:
		PR_ERROR("Invalid backend specified\n");
		return -1;
	}

	/* At this point we should have a device-tree loaded. We want
	 * to walk the tree and disabled nodes we don't care about
	 * prior to probing. */
	for_each_class_target("pib", pib) {
		int proc_index = pib->index;

		if (processorsel[proc_index]) {
			enable_dn(pib->dn);
			if (!find_target_class("chiplet"))
				continue;
			for_each_class_target("chiplet", chip) {
				if (chip->dn->parent != pib->dn)
					continue;
				int chip_index = chip->index;
				if (chipsel[proc_index][chip_index]) {
					enable_dn(chip->dn);
					if (!find_target_class("thread"))
						continue;
					for_each_class_target("thread", thread) {
						if (thread->dn->parent != chip->dn)
							continue;

						int thread_index = thread->index;
						if (threadsel[proc_index][chip_index][thread_index])
							enable_dn(thread->dn);
						else
							disable_dn(thread->dn);
					}
				} else
					disable_dn(chip->dn);
			}
		} else
			disable_dn(pib->dn);
	}

	for_each_class_target("fsi", fsi) {
		int index = fsi->index;
		if (processorsel[index])
			enable_dn(fsi->dn);
		else
			disable_dn(fsi->dn);
	}

	return 0;
}

void print_target(struct dt_node *dn, int level)
{
	int i;
	struct dt_node *next;
	struct dt_property *p;
	char *status = "";

	p = dt_find_property(dn, "status");
	if (p)
		status = p->prop;

	if (!strcmp(status, "disabled"))
		return;

	if (strcmp(status, "hidden")) {
		struct target *target;

		for (i = 0; i < level; i++)
			printf("    ");

		target = dn->target;
		if (target) {
			char c = 0;
			if (!strcmp(target->class, "pib"))
				c = 'p';
			else if (!strcmp(target->class, "chiplet"))
				c = 'c';
			else if (!strcmp(target->class, "thread"))
				c = 't';

			if (c)
				printf("%c%d: %s\n", c, target->index, target->name);
			else
				printf("%s\n", target->name);
		}
	}

	list_for_each(&dn->children, next, list)
		print_target(next, level + 1);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	uint8_t *buf;
	struct target *target;

	if (parse_options(argc, argv))
		return 1;

	/* Disable unselected targets */
	if (target_select())
		return 1;

	target_probe();

	switch(cmd) {
	case GETCFAM:
		rc = for_each_target("fsi", getcfam, &cmd_args[0], NULL);
		break;
	case PUTCFAM:
		rc = for_each_target("fsi", putcfam, &cmd_args[0], &cmd_args[1]);
		break;
	case GETSCOM:
		rc = for_each_target("pib", getscom, &cmd_args[0], NULL);
		break;
	case PUTSCOM:
		rc = for_each_target("pib", putscom, &cmd_args[0], &cmd_args[1]);
		break;
	case GETMEM:
                buf = malloc(cmd_args[1]);
                assert(buf);
		for_each_class_target("adu", target) {
			if (!adu_getmem(target, cmd_args[0], buf, cmd_args[1])) {
				if (write(STDOUT_FILENO, buf, cmd_args[1]) < 0)
					PR_ERROR("Unable to write stdout.\n");
				else
					rc++;
			} else
				PR_ERROR("Unable to read memory.\n");

			/* We only ever care about getting memory from a single processor */
			break;
		}

		free(buf);
		break;
	case PUTMEM:
                rc = putmem(cmd_args[0]);
                printf("Wrote %d bytes starting at 0x%016" PRIx64 "\n", rc, cmd_args[0]);
		break;
	case GETGPR:
		rc = for_each_target("thread", getprocreg, &cmd_args[0], NULL);
		break;
	case PUTGPR:
		rc = for_each_target("thread", putprocreg, &cmd_args[0], &cmd_args[1]);
		break;
	case GETNIA:
		cmd_args[0] = REG_NIA;
		rc = for_each_target("thread", getprocreg, &cmd_args[0], NULL);
		break;
	case PUTNIA:
		cmd_args[1] = cmd_args[0];
		cmd_args[0] = REG_NIA;
		rc = for_each_target("thread", putprocreg, &cmd_args[0], &cmd_args[1]);
		break;
	case GETSPR:
		cmd_args[0] += REG_R31;
		rc = for_each_target("thread", getprocreg, &cmd_args[0], NULL);
		break;
	case PUTSPR:
		cmd_args[0] += REG_R31;
		rc = for_each_target("thread", putprocreg, &cmd_args[0], &cmd_args[1]);
		break;
	case GETMSR:
		cmd_args[0] = REG_MSR;
		rc = for_each_target("thread", getprocreg, &cmd_args[0], NULL);
		break;
	case PUTMSR:
		cmd_args[1] = cmd_args[0];
		cmd_args[0] = REG_MSR;
		rc = for_each_target("thread", putprocreg, &cmd_args[0], &cmd_args[1]);
		break;
	case THREADSTATUS:
		rc = for_each_target("pib", print_proc_thread_status, NULL, NULL);
		break;
	case START:
		rc = for_each_target("thread", start_thread, NULL, NULL);
		break;
	case STEP:
		rc = for_each_target("thread", step_thread, &cmd_args[0], NULL);
		break;
	case STOP:
		rc = for_each_target("thread", stop_thread, NULL, NULL);
		break;
	case SRESET:
		rc = for_each_target("thread", sreset_thread, NULL, NULL);
		break;
	case PROBE:
		rc = 1;
		print_target(dt_root, 0);
		printf("\nNote that only selected targets will be shown above. If none are shown\n"
		       "try adding '-a' to select all targets\n");
		break;
	case HTM_GO:
	case HTM_START:
		rc = run_htm_start();
		break;
	case HTM_STOP:
		rc = run_htm_stop();
		break;
	case HTM_STATUS:
		rc = run_htm_status();
		break;
	case HTM_RESET:
		rc = run_htm_reset();
		break;
	case HTM_DUMP:
		rc = run_htm_dump();
		break;
	case HTM_TRACE:
		rc = run_htm_trace();
		break;
	case HTM_ANALYSE:
		rc = run_htm_analyse();
		break;
	default:
		PR_ERROR("Unsupported command\n");
		break;
	}

	if (rc <= 0) {
                printf("No valid targets found or specified. Try adding -p/-c/-t options to specify a target.\n");
                printf("Alternatively run %s -a probe to get a list of all valid targets\n", argv[0]);
		rc = 1;
	} else
		rc = 0;

	if (backend == FSI)
		fsi_destroy(NULL);

	return rc;
}
