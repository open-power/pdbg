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

#include <ccan/array_size/array_size.h>

#include <config.h>

#include <libpdbg.h>

#include "main.h"
#include "bitutils.h"
#include "cfam.h"
#include "scom.h"
#include "reg.h"
#include "ring.h"
#include "mem.h"
#include "thread.h"
#include "htm.h"
#include "options.h"

#undef PR_DEBUG
#define PR_DEBUG(...)
#define PR_ERROR(x, args...)					\
	fprintf(stderr, "%s: " x, __FUNCTION__, ##args)

#define THREADS_PER_CORE	8

static enum backend backend = KERNEL;

static char const *device_node;
static int i2c_addr = 0x50;

#define MAX_PROCESSORS 16
#define MAX_CHIPS 24
#define MAX_THREADS THREADS_PER_CORE

static int **processorsel[MAX_PROCESSORS];
static int *chipsel[MAX_PROCESSORS][MAX_CHIPS];
static int threadsel[MAX_PROCESSORS][MAX_CHIPS][MAX_THREADS];

static int handle_probe(int optind, int argc, char *argv[]);

static struct {
	const char *name;
	const char *args;
	const char *desc;
	int (*fn)(int, int, char **);
} actions[] = {
	{ "getcfam", "<address>", "Read system cfam", &handle_cfams },
	{ "putcfam", "<address> <value> [<mask>]", "Write system cfam", &handle_cfams },
	{ "getscom", "<address>", "Read system scom", &handle_scoms },
	{ "putscom", "<address> <value> [<mask>]", "Write system scom", &handle_scoms },
	{ "getmem",  "<address> <count>", "Read system memory", &handle_mem },
	{ "putmem",  "<address>", "Write to system memory", &handle_mem },
	{ "getgpr",  "<gpr>", "Read General Purpose Register (GPR)", &handle_gpr },
	{ "putgpr",  "<gpr> <value>", "Write General Purpose Register (GPR)", &handle_gpr },
	{ "getnia",  "", "Get Next Instruction Address (NIA)", &handle_nia },
	{ "putnia",  "<value>", "Write Next Instrution Address (NIA)", &handle_nia },
	{ "getspr",  "<spr>", "Get Special Purpose Register (SPR)", &handle_spr },
	{ "putspr",  "<spr> <value>", "Write Special Purpose Register (SPR)", &handle_spr },
	{ "getmsr",  "", "Get Machine State Register (MSR)", &handle_msr },
	{ "putmsr",  "<value>", "Write Machine State Register (MSR)", &handle_msr },
	{ "getring", "<addr> <len>", "Read a ring. Length must be correct", &handle_getring },
	{ "start",   "", "Start thread", &thread_start },
	{ "step",    "<count>", "Set a thread <count> instructions", &thread_step },
	{ "stop",    "", "Stop thread", &thread_stop },
	{ "threadstatus", "", "Print the status of a thread", &thread_status_print },
	{ "sreset",  "", "Reset", &thread_sreset },
	{ "htm_start", "", "[deprecated use 'htm nest start'] Start Nest HTM", &run_htm_start },
	{ "htm_stop", "", "[deprecated use 'htm nest stop'] Stop Nest HTM", &run_htm_stop },
	{ "htm_status", "", "[deprecated use 'htm nest status'] Print the status of HTM", &run_htm_status },
	{ "htm_reset", "", "[deprecated use 'htm nest reset'] Reset the HTM facility", &run_htm_reset },
	{ "htm_dump", "", "[deprecated use 'htm nest dump'] Dump HTM buffer to file", &run_htm_dump },
	{ "htm_trace", "" , "[deprecated use 'htm nest trace'] Configure and start tracing with HTM", &run_htm_trace },
	{ "htm_analyse", "", "[derepcated use 'htm nest analyse'] Stop and dump buffer to file", &run_htm_analyse },
	{ "htm", "(core | nest) (start | stop | status | reset | dump | trace | analyse", "Hardware Trace Macro", &run_htm },
	{ "probe", "", "", &handle_probe },
};

static void print_usage(char *pname)
{
	int i;

	printf("Usage: %s [options] command ...\n\n", pname);
	printf(" Options:\n");
	printf("\t-p, --processor=processor-id\n");
	printf("\t-c, --chip=core-id\n");
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
	for (i = 0; i < ARRAY_SIZE(actions); i++)
		printf("\t%s %s\n", actions[i].name, actions[i].args);
}

static bool parse_options(int argc, char *argv[])
{
	int c;
	bool opt_error = true;
	static int current_processor = INT_MAX, current_chip = INT_MAX, current_thread = INT_MAX;
	struct option long_opts[] = {
		{"all",			no_argument,		NULL,	'a'},
		{"backend",		required_argument,	NULL,	'b'},
		{"chip",		required_argument,	NULL,	'c'},
		{"device",		required_argument,	NULL,	'd'},
		{"help",		no_argument,		NULL,	'h'},
		{"processor",		required_argument,	NULL,	'p'},
		{"slave-address",	required_argument,	NULL,	's'},
		{"thread",		required_argument,	NULL,	't'},
		{"version",		no_argument,		NULL,	'V'},
		{NULL,			0,			NULL,     0}
	};
	char *endptr;

	do {
		c = getopt_long(argc, argv, "+ab:c:d:hp:s:t:V", long_opts, NULL);
		if (c == -1)
			break;

		switch(c) {
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
			current_processor = strtoul(optarg, &endptr, 0);
			opt_error = (errno || *endptr != '\0');
			if (!opt_error) {
				if (current_processor >= MAX_PROCESSORS)
					opt_error = true;
				else
					processorsel[current_processor] = &chipsel[current_processor][0];
			}
			break;

		case 'c':
			errno = 0;
			current_chip = strtoul(optarg, &endptr, 0);
			opt_error = (errno || *endptr != '\0');
			if (!opt_error) {
				if (current_chip >= MAX_CHIPS)
					opt_error = true;
				else
					chipsel[current_processor][current_chip] = &threadsel[current_processor][current_chip][0];
			}
			break;

		case 't':
			errno = 0;
			current_thread = strtoul(optarg, &endptr, 0);
			opt_error = (errno || *endptr != '\0');
			if (!opt_error) {
				if (current_thread >= MAX_THREADS)
					opt_error = true;
				else
					threadsel[current_processor][current_chip][current_thread] = 1;
			}
			break;

		case 'b':
			opt_error = false;
			if (strcmp(optarg, "fsi") == 0) {
				backend = FSI;
				device_node = "p9w";
			} else if (strcmp(optarg, "i2c") == 0) {
				backend = I2C;
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
			i2c_addr = strtoull(optarg, &endptr, 0);
			opt_error = (errno || *endptr != '\0');
			break;

		case 'V':
			errno = 0;
			printf("%s (commit %s)\n", PACKAGE_STRING, GIT_SHA1);
			exit(1);
			break;

		case '?':
		case 'h':
			opt_error = true;
			break;
		}
	} while (c != EOF && !opt_error);

	if (opt_error)
		print_usage(argv[0]);

	return opt_error;
}

void target_select(struct pdbg_target *target)
{
	/* We abuse the private data pointer atm to indicate the target is
	 * selected */
	pdbg_target_priv_set(target, (void *) 1);
}

void target_unselect(struct pdbg_target *target)
{
	pdbg_target_priv_set(target, NULL);
}

bool target_selected(struct pdbg_target *target)
{
	return (bool) pdbg_target_priv(target);
}

/* Returns the sum of return codes. This can be used to count how many targets the callback was run on. */
int for_each_child_target(char *class, struct pdbg_target *parent,
				 int (*cb)(struct pdbg_target *, uint32_t, uint64_t *, uint64_t *),
				 uint64_t *arg1, uint64_t *arg2)
{
	int rc = 0;
	struct pdbg_target *target;
	uint32_t index;
	enum pdbg_target_status status;

	pdbg_for_each_target(class, parent, target) {
		if (!target_selected(target))
			continue;

		index = pdbg_target_index(target);
		assert(index != -1);
		pdbg_target_probe(target);
		status = pdbg_target_status(target);
		if (status != PDBG_TARGET_ENABLED)
			continue;

		rc += cb(target, index, arg1, arg2);
	}

	return rc;
}

int for_each_target(char *class, int (*cb)(struct pdbg_target *, uint32_t, uint64_t *, uint64_t *), uint64_t *arg1, uint64_t *arg2)
{
	struct pdbg_target *target;
	uint32_t index;
	enum pdbg_target_status status;
	int rc = 0;

	pdbg_for_each_class_target(class, target) {
		if (!target_selected(target))
			continue;

		index = pdbg_target_index(target);
		assert(index != -1);
		pdbg_target_probe(target);
		status = pdbg_target_status(target);
		if (status != PDBG_TARGET_ENABLED)
			continue;

		rc += cb(target, index, arg1, arg2);
	}

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

static int target_selection(void)
{
	struct pdbg_target *fsi, *pib, *chip, *thread;

	switch (backend) {
	case I2C:
		pdbg_targets_init(&_binary_p8_i2c_dtb_o_start);
		break;

	case FSI:
		if (device_node == NULL) {
			PR_ERROR("FSI backend requires a device type\n");
			return -1;
		}
		if (!strcmp(device_node, "p8"))
			pdbg_targets_init(&_binary_p8_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9w"))
			pdbg_targets_init(&_binary_p9w_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9r"))
			pdbg_targets_init(&_binary_p9r_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9z"))
			pdbg_targets_init(&_binary_p9z_fsi_dtb_o_start);
		else {
			PR_ERROR("Invalid device type specified\n");
			return -1;
		}
		break;

	case KERNEL:
		pdbg_targets_init(&_binary_p9_kernel_dtb_o_start);
		break;

	case FAKE:
		pdbg_targets_init(&_binary_fake_dtb_o_start);
		break;

	case HOST:
		if (device_node == NULL) {
			PR_ERROR("Host backend requires a device type\n");
			return -1;
		}
		if (!strcmp(device_node, "p8"))
			pdbg_targets_init(&_binary_p8_host_dtb_o_start);
		else if (!strcmp(device_node, "p9"))
			pdbg_targets_init(&_binary_p9_host_dtb_o_start);
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
	pdbg_for_each_class_target("pib", pib) {
		int proc_index = pdbg_target_index(pib);

		if (backend == I2C && device_node)
			pdbg_set_target_property(pib, "bus", device_node, strlen(device_node) + 1);

		if (processorsel[proc_index]) {
			target_select(pib);
			pdbg_for_each_target("core", pib, chip) {
				int chip_index = pdbg_target_index(chip);
				target_select(chip);
				if (chipsel[proc_index][chip_index]) {
					pdbg_for_each_target("thread", chip, thread) {
						int thread_index = pdbg_target_index(thread);
						target_select(thread);
						if (!threadsel[proc_index][chip_index][thread_index])
							target_unselect(thread);
					}
				} else
					target_unselect(chip);
			}

			/* This is kinda broken as we're overloading what '-c'
			 * means - it's now up to each command to select targets
			 * based on core/chiplet. We really need a better
			 * solution to target selection. */
			pdbg_for_each_target("chiplet", pib, chip) {
				int chip_index = pdbg_target_index(chip);
				if (chipsel[proc_index][chip_index]) {
					target_select(chip);
				} else
					target_unselect(chip);
			}
		} else
			target_unselect(pib);
	}

	pdbg_for_each_class_target("fsi", fsi) {
		int index = pdbg_target_index(fsi);
		if (processorsel[index])
			target_select(fsi);
		else
			target_unselect(fsi);
	}

	return 0;
}

void print_target(struct pdbg_target *target, int level)
{
	int i;
	struct pdbg_target *next;
	enum pdbg_target_status status;

	/* Did we want to probe this target? */
	if (!target_selected(target))
		return;

	pdbg_target_probe(target);

	/* Does this target actually exist? */
	status = pdbg_target_status(target);
	if (status != PDBG_TARGET_ENABLED)
		return;

	for (i = 0; i < level; i++)
		printf("    ");

	if (target) {
		char c = 0;
		if (!strcmp(pdbg_target_class_name(target), "pib"))
			c = 'p';
		else if (!strcmp(pdbg_target_class_name(target), "core"))
			c = 'c';
		else if (!strcmp(pdbg_target_class_name(target), "thread"))
			c = 't';

		if (c)
			printf("%c%d: %s\n", c, pdbg_target_index(target), pdbg_target_name(target));
		else
			printf("%s\n", pdbg_target_name(target));
	}

	pdbg_for_each_child_target(target, next) {
		print_target(next, level + 1);
	}
}

static int handle_probe(int optind, int argc, char *argv[])
{
	struct pdbg_target *target;

	pdbg_for_each_class_target("pib", target)
		print_target(target, 0);

	printf("\nNote that only selected targets will be shown above. If none are shown\n"
			"try adding '-a' to select all targets\n");
	return 1;
}

int main(int argc, char *argv[])
{
	int i, rc = 0;

	backend = default_backend();
	device_node = default_target(backend);

	if (parse_options(argc, argv))
		return 1;

	if (!backend_is_possible(backend)) {
		fprintf(stderr, "Backend not possible\nUse: ");
		print_backends(stderr);
		print_usage(argv[0]);
		return 1;
	}

	if (!target_is_possible(backend, device_node)) {
		fprintf(stderr, "Target %s not possible\n",
			device_node ? device_node : "(none)");
		print_targets(stderr);
		print_usage(argv[0]);
		return 1;
	}

	if (optind >= argc) {
		print_usage(argv[0]);
		return 1;
	}

	/* Disable unselected targets */
	if (target_selection())
		return 1;

	for (i = 0; i < ARRAY_SIZE(actions); i++) {
		if (strcmp(argv[optind], actions[i].name) == 0) {
			rc = actions[i].fn(optind, argc, argv);
			break;
		}
	}

	if (i == ARRAY_SIZE(actions)) {
		PR_ERROR("Unsupported command: %s\n", argv[optind]);
		print_usage(argv[0]);
		return 1;
	}

	if (rc <= 0) {
                printf("No valid targets found or specified. Try adding -p/-c/-t options to specify a target.\n");
                printf("Alternatively run %s -a probe to get a list of all valid targets\n", argv[0]);
		rc = 1;
	} else
		rc = 0;

	//if (backend == FSI)
		//fsi_destroy(NULL);

	return rc;
}
