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

#include <backend.h>
#include <operations.h>
#include <target.h>

#include <config.h>

#include "bitutils.h"

enum command { GETCFAM = 1, PUTCFAM, GETSCOM, PUTSCOM,	\
	       GETMEM, PUTMEM, GETGPR, GETNIA, GETSPR,	\
	       GETMSR, PUTGPR, PUTNIA, PUTSPR, PUTMSR,	\
	       STOPCHIP, STARTCHIP, THREADSTATUS, STEP, \
	       PROBE, GETVMEM };

#define MAX_CMD_ARGS 3
enum command cmd = 0;
static int cmd_arg_count = 0;
static int cmd_min_arg_count = 0;
static int cmd_max_arg_count = 0;

/* At the moment all commands only take some kind of number */
static uint64_t cmd_args[MAX_CMD_ARGS];

enum backend { FSI, I2C };
static enum backend backend = I2C;

static char const *device_node;
static int i2c_addr = 0x50;

#define MAX_TARGETS 400
static struct target targets[MAX_TARGETS];

#define MAX_PROCESSORS 16
#define MAX_CHIPS 16
#define MAX_THREADS THREADS_PER_CORE

static int **processor[MAX_PROCESSORS];
static int *chip[MAX_PROCESSORS][MAX_CHIPS];
static int thread[MAX_PROCESSORS][MAX_CHIPS][MAX_THREADS];

struct target_class cfams = {
	.name = "CFAMS",
};

struct target_class processors = {
	.name = "Processors",
};

struct target_class chiplets = {
	.name = "Chiplets",
};

struct target_class threads = {
	.name = "Threads",
};

#define for_each_thread(x) list_for_each(&threads.targets, x, class_link)
#define for_each_chiplet(x) list_for_each(&chiplets.targets, x, class_link)
#define for_each_processor(x) list_for_each(&processors.targets, x, class_link)
#define for_each_cfam(x) list_for_each(&cfams.targets, x, class_link)

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
	printf("\t\ti2c:\tThe default backend which goes via I2C.\n");
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
	printf("\tstartchip\n");
	printf("\tstep <count>\n");
	printf("\tstopchip\n");
	printf("\tthreadstatus\n");
	printf("\tprobe\n");
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
	} else if (strcmp(optarg, "startchip") == 0) {
		cmd = STARTCHIP;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "step") == 0) {
		cmd = STEP;
		cmd_min_arg_count = 1;
	} else if (strcmp(optarg, "stopchip") == 0) {
		cmd = STOPCHIP;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "threadstatus") == 0) {
		cmd = THREADSTATUS;
		cmd_min_arg_count = 0;
	} else if (strcmp(optarg, "probe") == 0) {
		cmd = PROBE;
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
				processor[current_processor] = &chip[current_processor][0];
				for (current_chip = 0; current_chip < MAX_CHIPS; current_chip++) {
					chip[current_processor][current_chip] = &thread[current_processor][current_chip][0];
					for (current_thread = 0; current_thread < MAX_THREADS; current_thread++)
						thread[current_processor][current_chip][current_thread] = 1;
				}
			}
			break;

		case 'p':
			errno = 0;
			current_processor = strtoul(optarg, NULL, 0);
			if (current_processor >= MAX_PROCESSORS)
				errno = -1;
			else
				processor[current_processor] = &chip[current_processor][0];
			opt_error = errno;
			break;

		case 'c':
			errno = 0;
			current_chip = strtoul(optarg, NULL, 0);
			if (current_chip >= MAX_CHIPS)
				errno = -1;
			else
				chip[current_processor][current_chip] = &thread[current_processor][current_chip][0];
			opt_error = errno;
			break;

		case 't':
			errno = 0;
			current_thread = strtoul(optarg, NULL, 0);
			if (current_thread >= MAX_THREADS)
				errno = -1;
			else
				thread[current_processor][current_chip][current_thread] = 1;
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

/*
 * Add a given target to the class
 */
void target_class_add(struct target_class *class, struct target *target, int index)
{
	list_add_tail(&class->targets, &target->class_link);
	target->index = index;
}

/*
 * Initialises an I2C based backend. Returns the number of targets
 * added on success.
 */
static int i2c_backend_targets_init(const char *bus, int addr)
{
	int i, cfam_count;

	CHECK_ERR(i2c_target_init(&targets[0], "BMC I2C Backend", NULL, device_node, i2c_addr));
	CHECK_ERR(opb_target_init(&targets[1], "PIB2OPB", 0x20010, &targets[0]));

	if (processor[0])
		target_class_add(&processors, &targets[0], 0);

	/* Probe cascaded CFAMs on hMFSI ports and add FSI2PIB bridges */
	cfam_count = hmfsi_target_probe(&targets[1], &targets[2], MAX_TARGETS);
	for (i = 0; i < cfam_count; i++) {
		/* Skip prcessors that aren't selected */
		if (!(processor[i + 1]))
			continue;

		fsi2pib_target_init(&targets[2 + cfam_count + i], "FSI2PIB", FSI2PIB_BASE, &targets[2 + i]);

		/* CFAM index should match processor index even though
		 * we don't support cfam 0 on i2c */
		target_class_add(&cfams, &targets[2 + i], i + 1);
		target_class_add(&processors, &targets[2 + cfam_count + i], i + 1);
	}

	return 2 + 2*cfam_count;
}

static int fsi_backend_targets_init(void)
{
	struct target *cfam;
	int i, cfam_count;
	enum fsi_system_type type;

	if (!strcmp(device_node, "p8"))
		type = FSI_SYSTEM_P8;
	else if (!strcmp(device_node, "p9w") || !strcmp(device_node, "witherspoon"))
		type = FSI_SYSTEM_P9W;
	else if (!strcmp(device_node, "p9r") || !strcmp(device_node, "romulus"))
		type = FSI_SYSTEM_P9R;
	else if (!strcmp(device_node, "p9z") || !strcmp(device_node, "zaius"))
		type = FSI_SYSTEM_P9Z;
	else {
		PR_ERROR("Unrecognized FSI system type %s\n", device_node);
		return -1;
	}

	fsi_target_init(&targets[0], "BMC FSI Backend", type, NULL);

	/* The backend is directly connected to a processor CFAM */
	if (processor[0])
		target_class_add(&cfams, &targets[0], 0);
	cfam_count = 1;

	/* Probe cascaded CFAMs on hMFSI ports */
	cfam_count += hmfsi_target_probe(&targets[0], &targets[1], MAX_TARGETS);
	for (i = 1; i < cfam_count; i++)
		if (processor[i])
			target_class_add(&cfams, &targets[i], i);

	/* Add a FSI2PIB bridges for each CFAM */
	i = 0;
	for_each_cfam(cfam) {
		fsi2pib_target_init(&targets[cfam_count + i], "FSI2PIB", FSI2PIB_BASE, cfam);

		if (processor[cfam->index])
			target_class_add(&processors, &targets[cfam_count + i], cfam->index);
		i++;
	}

	return 2*cfam_count;
}

static int backend_init(void)
{
	int i, j, target_count = 0, chiplet_count = 0, thread_count = 0;
	struct target *target, *processor, *chiplet;

	list_head_init(&cfams.targets);
	list_head_init(&processors.targets);
	list_head_init(&chiplets.targets);
	list_head_init(&threads.targets);

	switch (backend) {
	case I2C:
		if ((target_count = i2c_backend_targets_init(device_node, i2c_addr)) < 0) {
			PR_ERROR("Unable to I2C initialise backend\n");
			return -1;
		}
		break;

	case FSI:
		if ((target_count = fsi_backend_targets_init()) < 0) {
			PR_ERROR("Unable to FSI initialise backend\n");
			return -1;
		}
		break;

	default:
		PR_ERROR("Invalid backend specified\n");
		return -1;
	}

	target = &targets[target_count];
	j = 0;
	for_each_processor(processor) {
		chiplet_count += chiplet_target_probe(processor, target, MAX_TARGETS - target_count);
		for (i = 0; j < chiplet_count; i++, j++) {
			/* Skip chips that aren't selected */
			if (chip[processor->index][j])
				target_class_add(&chiplets, target, i);

			target++;
		}
	}

	target_count += chiplet_count;
	target = &targets[target_count];
	j = 0;
	for_each_chiplet(chiplet) {
		thread_count += thread_target_probe(chiplet, target, MAX_TARGETS - target_count);
		for (i = 0; j < thread_count; i++, j++) {
			target_class_add(&threads, target, i);
			if (!chip[chiplet->next->index][chiplet->index][j])
				target->status |= THREAD_STATUS_DISABLED;
			target++;
		}
	}

	return 0;
}


static void dump_targets(int indent, struct target *target)
{
	int i;
	struct target *child;

	for (i = 0; i < indent; i++)
		printf("    ");

	printf("%s@%llx\n", target->name, target->base);

	if (!list_empty(&target->children))
		list_for_each(&target->children, child, link)
			dump_targets(indent + 1, child);
}

/* Dump all processors and chiplets */
static uint64_t probe(void)
{
	struct target *cfam, *processor, *chiplet, *thread;
	int i, j;

	printf("Overall System Topology\n");
	printf("=======================\n\n");
	dump_targets(0, &targets[0]);

	i = 0;
	printf("\nCFAMs Present\n");
	printf("=============\n\n");
	for_each_cfam(cfam)
		printf("%d: %s@%llx\n", i++, cfam->name, cfam->base);

	i = 0;
	printf("\nProcessors/Chiplets Present\n");
	printf("===========================\n\n");
	for_each_processor(processor) {
		printf("Processor-ID %d: %s@%llx\n", processor->index, processor->name, processor->base);
		j = 0;
		for_each_chiplet(chiplet)
			if (chiplet->next == processor) {
				printf("\tChiplet-ID %d: %s@%llx\n", chiplet->index, chiplet->name, chiplet->base);
				for_each_thread(thread)
					if (thread->next == chiplet)
						printf("\t\tThread-ID %d: %s@%llx\n", thread->index, thread->name, thread->base);
			}
	}

	return 0;
}

static int filter_parent(struct target *target, void *priv)
{
	struct target *parent = priv;

	return target->next == parent;
}

/*
 * Call cb() on each member of the target class when filter() returns
 * true. Returns either the result from cb() (should be -ve on error)
 * or the number of time cb() was called.
 */
static int for_each_class_call(struct target_class *class, int (*filter)(struct target *, void *),
			       int (*cb)(struct target *, uint64_t, uint64_t *),
			       void *priv, uint64_t addr, uint64_t *data)
{
	struct target *target;
	int rc, count = 0;

	list_for_each(&class->targets, target, class_link)
		if ((!filter || filter(target, priv)) && !(target->status & THREAD_STATUS_DISABLED)) {
			count++;
			if((rc = cb(target, addr, data)))
				return rc;
		}

	return count;
}

/* Same as above but calls cb() on all targets rather than just active ones */
static int for_each_class_call_all(struct target_class *class, int (*filter)(struct target *, void *),
				   int (*cb)(struct target *, uint64_t, uint64_t *),
				   void *priv, uint64_t addr, uint64_t *data)
{
	struct target *target;
	int rc, count = 0;

	list_for_each(&class->targets, target, class_link)
		if (!filter || filter(target, priv)) {
			count++;
			if((rc = cb(target, addr, data)))
				return rc;
		}

	return count;
}

static int print_thread_status(struct target *thread, uint64_t addr, uint64_t *data)
{
	uint64_t status = chiplet_thread_status(thread);
	static int last_index = -1;
	int i;

	if (thread->index <= last_index)
		last_index = -1;

	for (i = last_index + 1; i < thread->index; i++)
		printf("  ");

	last_index = thread->index;

	switch (status)
	{
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

	default:
		printf(" U");
		break;
	}

	return 0;
}

static int print_chiplet_thread_status(struct target *chiplet, uint64_t addr, uint64_t *data)
{
	printf("c%02d:", chiplet->index);

	for_each_class_call(&threads, filter_parent, print_thread_status, chiplet, 0, NULL);
	printf("\n");

	return 0;
}

static int getaddr(struct target *target, uint64_t addr, uint64_t *data)
{
	int rc;

	if ((rc = read_target(target, addr, data)))
		PR_ERROR("Error reading register\n");
	else
		printf("p%d:0x%llx: 0x%016llx\n", target->index, addr, *data);

	return rc;
}

static int putaddr(struct target *target, uint64_t addr, uint64_t *data)
{
	uint64_t val, mask = data[1];
	int rc;

	if (mask != -1ULL) {
		if ((rc = read_target(target, addr, &val))) {
			PR_ERROR("Unable to read register\n");
			return rc;
		}

		val &= ~mask;
		val |= data[0] & mask;
	} else
		val = data[0];

	if ((rc = write_target(target, addr, val)))
		PR_ERROR("Error writing register\n");
	else
		printf("p%d:0x%llx: 0x%016llx\n", target->index, addr, val);

	return rc;
}

#define START_CHIP 1
#define STEP_CHIP 2
#define STOP_CHIP 3
static int startstopstep_thread(struct target *thread, uint64_t action, uint64_t *data)
{
	uint64_t status;

	if (action == START_CHIP)
		return ram_start_thread(thread);
	else if (action == STEP_CHIP) {
		status = chiplet_thread_status(thread);
		if (!(GETFIELD(THREAD_STATUS_QUIESCE, status) && GETFIELD(THREAD_STATUS_ACTIVE, status))) {
			PR_ERROR("Thread %d not stopped. Use stopchip first.\n", thread->index);

			/* Return 0 so we continue stepping other threads if requested */
			return 0;
		}

		return ram_step_thread(thread, *data);
	} else
		return ram_stop_thread(thread);
}

/*
 * Start/stop all threads on the chip.
 */
static int startstopstep_chip(struct target *chiplet, uint64_t action, uint64_t *data)
{
	if (action == STEP_CHIP)
		for_each_class_call(&threads, filter_parent, startstopstep_thread, chiplet, action, data);
	else
		for_each_class_call_all(&threads, filter_parent, startstopstep_thread, chiplet, action, data);

	return 0;
}

static int check_thread_status(struct target *thread, uint64_t unused, uint64_t *unused1)
{
	uint64_t status = chiplet_thread_status(thread);

	if (status & THREAD_STATUS_QUIESCE)
		return 0;
	else
		return -1;
}

static int check_state(struct target *thread)
{
	struct target *chiplet = thread->next;

	if (for_each_class_call_all(&threads, filter_parent, check_thread_status, chiplet, 0, NULL) < 0) {
		PR_ERROR("Not all threads are quiesced. Use the stopchip command"
			 " first to ensure all chiplet threads are quiesced\n");
		return -1;
	}

	if (!((thread->status & THREAD_STATUS_ACTIVE) && (thread->status & THREAD_STATUS_QUIESCE))) {
		PR_ERROR("Skipping thread %d as it is not active.\n", thread->index);
		return -2;
	}

	return 0;
}

#define REG_MEM -3ULL
#define REG_MSR -2ULL
#define REG_NIA -1ULL
#define REG_R31 31ULL

static int putreg(struct target *thread, uint64_t reg, uint64_t *data)
{
	int state;

	state = check_state(thread);
	if (state == -2)
		/* Return 0 so we keep attempting other non-sleeping
		 * threads on the chiplet if requested */
		return 0;
	else if (state)
		return state;

	if (reg == REG_NIA)
		if (ram_putnia(thread, *data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:nia = 0x%016llx\n", thread->next->index, thread->index, *data);
	else if (reg == REG_MSR)
		if (ram_putmsr(thread, *data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:msr = 0x%016llx\n", thread->next->index, thread->index, *data);
	else if (reg <= REG_R31)
		if (ram_putgpr(thread, reg, *data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:r%lld = 0x%016llx\n", thread->next->index, thread->index, reg, *data);
	else {
		reg -= 32;
		if (ram_putspr(thread, reg, *data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:spr%lld = 0x%016llx\n", thread->next->index, thread->index, reg, *data);
	}

	return 0;
}

static int getreg(struct target *thread, uint64_t reg, uint64_t *data)
{
	uint64_t addr = *data;
	int state;

	state = check_state(thread);
	if (state == -2)
		/* Return 0 so we keep attempting other non-sleeping
		 * threads on the chiplet if requested */
		return 0;
	else if (state)
		return state;

	if (reg == REG_NIA)
		if (ram_getnia(thread, data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:nia = 0x%016llx\n", thread->next->index, thread->index, *data);
	else if (reg == REG_MSR)
		if (ram_getmsr(thread, data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:msr = 0x%016llx\n", thread->next->index, thread->index, *data);
	else if (reg == REG_MEM)
		if (ram_getmem(thread, addr, data))
			PR_ERROR("Page fault reading memory\n");
		else
			printf("c%02d:t%d:0x%016llx = 0x%016llx\n", thread->next->index, thread->index, addr, *data);
	else if (reg <= REG_R31)
		if (ram_getgpr(thread, reg, data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:r%lld = 0x%016llx\n", thread->next->index, thread->index, reg, *data);
	else {
		reg -= 32;
		if (ram_getspr(thread, reg, data))
			PR_ERROR("Error reading register\n");
		else
			printf("c%02d:t%d:spr%lld = 0x%016llx\n", thread->next->index, thread->index, reg, *data);
	}

	return 0;
}

#define PUTMEM_BUF_SIZE 1024
static int putmem(uint64_t addr)
{
	uint8_t *buf;
	int read_size, rc = 0;
	struct target *target;

	/* It doesn't matter which processor we execute on so
	 * just use the primary one */
	if (list_empty(&processors.targets))
		return 0;

	target = list_entry(processors.targets.n.next, struct target, class_link);
	buf = malloc(PUTMEM_BUF_SIZE);
	assert(buf);
	do {
		read_size = read(STDIN_FILENO, buf, PUTMEM_BUF_SIZE);
		if (adu_putmem(target, addr, buf, read_size)) {
			rc = 0;
			PR_ERROR("Unable to write memory.\n");
			break;
		}

		rc += read_size;
	} while (read_size > 0);

	free(buf);
	return rc;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	uint64_t value = 0;
	uint8_t *buf;
	struct target *target;

	if (parse_options(argc, argv))
		return 1;

	if (backend_init())
		return 1;

	switch(cmd) {
	case GETCFAM:
		rc = for_each_class_call(&cfams, NULL, getaddr, NULL, cmd_args[0], &value);
		break;
	case GETSCOM:
		rc = for_each_class_call(&processors, NULL, getaddr, NULL, cmd_args[0], &value);
		break;
	case PUTCFAM:
		rc = for_each_class_call(&cfams, NULL, putaddr, NULL, cmd_args[0], &cmd_args[1]);
		break;
	case PUTSCOM:
		rc = for_each_class_call(&processors, NULL, putaddr, NULL, cmd_args[0], &cmd_args[1]);
		break;
	case GETMEM:
		/* It doesn't matter which processor we execute on so
		 * just use the primary one */
		if (list_empty(&processors.targets))
			break;

		target = list_entry(processors.targets.n.next, struct target, class_link);
		buf = malloc(cmd_args[1]);
		assert(buf);
		if (!adu_getmem(target, cmd_args[0], buf, cmd_args[1])) {
			rc = 1;
			write(STDOUT_FILENO, buf, cmd_args[1]);
		} else
			PR_ERROR("Unable to read memory.\n");

		free(buf);
		break;
	case PUTMEM:
		rc = putmem(cmd_args[0]);
		printf("Wrote %d bytes starting at 0x%016llx\n", rc, cmd_args[0]);
		break;
	case GETGPR:
		rc = for_each_class_call(&threads, NULL, getreg, NULL, cmd_args[0], &value);
		break;
	case PUTGPR:
		rc = for_each_class_call(&threads, NULL, putreg, NULL, cmd_args[0], &cmd_args[1]);
		break;
	case GETNIA:
		rc = for_each_class_call(&threads, NULL, getreg, NULL, REG_NIA, &value);
		break;
	case PUTNIA:
		rc = for_each_class_call(&threads, NULL, putreg, NULL, REG_NIA, &cmd_args[0]);
		break;
	case GETSPR:
		rc = for_each_class_call(&threads, NULL, getreg, NULL, cmd_args[0] + 32, &value);
		break;
	case PUTSPR:
		rc = for_each_class_call(&threads, NULL, putreg, NULL, cmd_args[0] + 32, &cmd_args[1]);
		break;
	case GETMSR:
		rc = for_each_class_call(&threads, NULL, getreg, NULL, REG_MSR, &value);
		break;
	case PUTMSR:
		rc = for_each_class_call(&threads, NULL, putreg, NULL, REG_MSR, &cmd_args[0]);
		break;
	case GETVMEM:
		rc = for_each_class_call(&threads, NULL, getreg, NULL, REG_MEM, &cmd_args[0]);
		break;
	case THREADSTATUS:
		printf("  t: 0 1 2 3 4 5 6 7\n");
		rc = for_each_class_call(&chiplets, NULL, print_chiplet_thread_status, NULL, 0, 0);
		printf("\nA - Active\nS - Sleep\nN - Nap\nD - Doze\nQ - Quiesced/Stopped\nU - Unknown\n");
		break;
	case STARTCHIP:
		rc = for_each_class_call(&chiplets, NULL, startstopstep_chip, NULL, START_CHIP, NULL);
		break;
	case STEP:
		rc = for_each_class_call(&chiplets, NULL, startstopstep_chip, NULL, STEP_CHIP, &cmd_args[0]);
		break;
	case STOPCHIP:
		rc = for_each_class_call(&chiplets, NULL, startstopstep_chip, NULL, STOP_CHIP, NULL);
		break;
	case PROBE:
		rc = 1;
		probe();
		break;
	}

	if (rc <= 0) {
		printf("No valid targets found or specified. Try adding -p/-c/-t options to specify a target.\n");
		printf("Alternatively run %s -a probe to get a list of all valid targets\n", argv[0]);
		rc = -1;
	} else
		rc = 0;

	/* TODO: We don't properly tear down all the targets yet */
	targets[0].destroy(&targets[0]);

	return rc;
}
