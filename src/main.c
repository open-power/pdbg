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

#include "bitutils.h"
#include "cfam.h"
#include "scom.h"
#include "reg.h"
#include "mem.h"
#include "thread.h"
#include "htm.h"

#undef PR_DEBUG
#define PR_DEBUG(...)
#define PR_ERROR(x, args...)					\
	fprintf(stderr, "%s: " x, __FUNCTION__, ##args)

#define THREADS_PER_CORE	8

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

static struct {
	const char *name;
	const char *args;
	const char *desc;
	int (*fn)(int, int, char **);
} actions[] = {
	{ "none yet", "nothing", "placeholder", NULL },
};

static void print_usage(char *pname)
{
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

static bool parse_command(int argc, char *argv[])
{
	int cmd_arg_idx = 0, i = optind;
	bool opt_error = true;
	char *endptr;

	/*
	 * In order to make the series changing this code cleaner,
	 * this is going to happen gradually.
	 * Eventually parse_command() will be deleted but so that this
	 * can be done over multiple patches neatly remove the error
	 * check here and print the usage later.
	 * We'll just have a lie a bit for now.
	 */
	if (i >= argc || !parse_cmd(argv[i]))
		return false;

	i++;
	while (i < argc && !opt_error) {
		if (cmd_arg_idx >= MAX_CMD_ARGS ||
			 (cmd && cmd_arg_idx >= cmd_max_arg_count))
			opt_error = true;
		else {
			errno = 0;
			cmd_args[cmd_arg_idx++] = strtoull(argv[i], &endptr, 0);
			opt_error = (errno || *endptr != '\0');
		}
		i++;
	}
	opt_error |= cmd_arg_idx < cmd_min_arg_count;
	if (opt_error)
		print_usage(argv[0]);

	cmd_arg_count = cmd_arg_idx;
	return opt_error;
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
		index = pdbg_target_index(target);
		assert(index != -1);
		status = pdbg_target_status(target);
		if (status == PDBG_TARGET_DISABLED || status == PDBG_TARGET_HIDDEN)
			continue;

		rc += cb(target, index, arg1, arg2);
	}

	return rc;
}

/* Call the given call back on each enabled target in the given class */
static int for_each_target(char *class, int (*cb)(struct pdbg_target *, uint32_t, uint64_t *, uint64_t *), uint64_t *arg1, uint64_t *arg2)
{
	struct pdbg_target *target;
	uint32_t index;
	enum pdbg_target_status status;
	int rc = 0;

	pdbg_for_each_class_target(class, target) {
		index = pdbg_target_index(target);
		assert(index != -1);
		status = pdbg_target_status(target);
		if (status == PDBG_TARGET_DISABLED || status == PDBG_TARGET_HIDDEN)
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
static int target_select(void)
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
		else if (!strcmp(device_node, "p9w") || !strcmp(device_node, "witherspoon"))
			pdbg_targets_init(&_binary_p9w_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9r") || !strcmp(device_node, "romulus"))
			pdbg_targets_init(&_binary_p9r_fsi_dtb_o_start);
		else if (!strcmp(device_node, "p9z") || !strcmp(device_node, "zaius"))
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
			pdbg_enable_target(pib);
			pdbg_for_each_target("core", pib, chip) {
				int chip_index = pdbg_target_index(chip);
				if (chipsel[proc_index][chip_index]) {
					pdbg_enable_target(chip);
					pdbg_for_each_target("thread", chip, thread) {
						int thread_index = pdbg_target_index(thread);
						if (threadsel[proc_index][chip_index][thread_index])
							pdbg_enable_target(thread);
						else
							pdbg_disable_target(thread);
					}
				} else
					pdbg_disable_target(chip);
			}
		} else
			pdbg_disable_target(pib);
	}

	pdbg_for_each_class_target("fsi", fsi) {
		int index = pdbg_target_index(fsi);
		if (processorsel[index])
			pdbg_enable_target(fsi);
		else
			pdbg_disable_target(fsi);
	}

	return 0;
}

void print_target(struct pdbg_target *target, int level)
{
	int i;
	struct pdbg_target *next;
	enum pdbg_target_status status;

	status = pdbg_target_status(target);
	if (status == PDBG_TARGET_DISABLED)
		return;

	if (status == PDBG_TARGET_ENABLED) {
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
	}

	pdbg_for_each_child_target(target, next)
		print_target(next, level + 1);
}

int main(int argc, char *argv[])
{
	struct pdbg_target *target;
	bool found = true;
	int i, rc = 0;
	uint8_t *buf;

	if (parse_options(argc, argv))
		return 1;

	if (optind >= argc) {
		print_usage(argv[0]);
		return 1;
	}

	/* Disable unselected targets */
	if (target_select())
		return 1;

	pdbg_target_probe();

	if (parse_command(argc, argv))
		return -1;

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
		pdbg_for_each_class_target("adu", target) {
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
		pdbg_for_each_class_target("pib", target)
			print_target(target, 0);

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
		found = false;
		break;
	}

	for (i = 0; i < ARRAY_SIZE(actions); i++) {
		if (strcmp(argv[optind], actions[i].name) == 0) {
			found = true;
			rc = actions[i].fn(optind, argc, argv);
			break;
		}
	}

	if (!found) {
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
