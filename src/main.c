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
#include <target.h>

#include "main.h"
#include "htm.h"
#include "options.h"
#include "optcmd.h"

#define PR_ERROR(x, args...) \
	pdbg_log(PDBG_ERROR, x, ##args)

#include "fake.dt.h"

#ifdef TARGET_ARM
#include "p8-i2c.dt.h"
#include "p8-fsi.dt.h"
#include "p9w-fsi.dt.h"
#include "p9r-fsi.dt.h"
#include "p9z-fsi.dt.h"
#include "p9-kernel.dt.h"
#endif

#ifdef TARGET_PPC
#include "p8-host.dt.h"
#include "p9-host.dt.h"
#endif

#define THREADS_PER_CORE	8

static enum backend backend = KERNEL;

static char const *device_node;
static int i2c_addr = 0x50;

#define MAX_PROCESSORS 64
#define MAX_CHIPS 24
#define MAX_THREADS THREADS_PER_CORE

#define MAX_LINUX_CPUS	(MAX_PROCESSORS * MAX_CHIPS * MAX_THREADS)

static int **processorsel[MAX_PROCESSORS];
static int *chipsel[MAX_PROCESSORS][MAX_CHIPS];
static int threadsel[MAX_PROCESSORS][MAX_CHIPS][MAX_THREADS];

static int probe(void);
static int release(void);

/* TODO: We are repeating ourselves here. A little bit more macro magic could
 * easily fix this but I was hesitant to introduce too much magic all at
 * once. */
extern struct optcmd_cmd
	optcmd_getscom, optcmd_putscom,	optcmd_getcfam, optcmd_putcfam,
	optcmd_getgpr, optcmd_putgpr, optcmd_getspr, optcmd_putspr,
	optcmd_getnia, optcmd_putnia, optcmd_getmsr, optcmd_putmsr,
	optcmd_getring, optcmd_start, optcmd_stop, optcmd_step,
	optcmd_threadstatus, optcmd_sreset, optcmd_regs, optcmd_probe,
	optcmd_getmem, optcmd_putmem;

static struct optcmd_cmd *cmds[] = {
	&optcmd_getscom, &optcmd_putscom, &optcmd_getcfam, &optcmd_putcfam,
	&optcmd_getgpr, &optcmd_putgpr, &optcmd_getspr, &optcmd_putspr,
	&optcmd_getnia, &optcmd_putnia, &optcmd_getmsr, &optcmd_putmsr,
	&optcmd_getring, &optcmd_start, &optcmd_stop, &optcmd_step,
	&optcmd_threadstatus, &optcmd_sreset, &optcmd_regs, &optcmd_probe,
	&optcmd_getmem, &optcmd_putmem,
};

/* Purely for printing usage text. We could integrate printing argument and flag
 * help into optcmd if desired. */
struct action {
	const char *name;
	const char *args;
	const char *desc;
};

static struct action actions[] = {
	{ "getgpr",  "<gpr>", "Read General Purpose Register (GPR)" },
	{ "putgpr",  "<gpr> <value>", "Write General Purpose Register (GPR)" },
	{ "getnia",  "", "Get Next Instruction Address (NIA)" },
	{ "putnia",  "<value>", "Write Next Instrution Address (NIA)" },
	{ "getspr",  "<spr>", "Get Special Purpose Register (SPR)" },
	{ "putspr",  "<spr> <value>", "Write Special Purpose Register (SPR)" },
	{ "getmsr",  "", "Get Machine State Register (MSR)" },
	{ "putmsr",  "<value>", "Write Machine State Register (MSR)" },
	{ "getring", "<addr> <len>", "Read a ring. Length must be correct" },
	{ "start",   "", "Start thread" },
	{ "step",    "<count>", "Set a thread <count> instructions" },
	{ "stop",    "", "Stop thread" },
	{ "htm", "core|nest start|stop|status|dump|record", "Hardware Trace Macro" },
	{ "release", "", "Should be called after pdbg work is finished" },
	{ "probe", "", "" },
	{ "getcfam", "<address>", "Read system cfam" },
	{ "putcfam", "<address> <value> [<mask>]", "Write system cfam" },
	{ "getscom", "<address>", "Read system scom" },
	{ "putscom", "<address> <value> [<mask>]", "Write system scom" },
	{ "getmem",  "<address> <count>", "Read system memory" },
	{ "putmem",  "<address>", "Write to system memory" },
	{ "threadstatus", "", "Print the status of a thread" },
	{ "sreset",  "", "Reset" },
	{ "regs",  "", "State" },
};

static void print_usage(char *pname)
{
	int i;

	printf("Usage: %s [options] command ...\n\n", pname);
	printf(" Options:\n");
	printf("\t-p, --processor=<0-%d>|<range>|<list>\n", MAX_PROCESSORS-1);
	printf("\t-c, --chip=<0-%d>|<range>|<list>\n", MAX_CHIPS-1);
	printf("\t-t, --thread=<0-%d>|<range>|<list>\n", MAX_THREADS-1);
#ifdef TARGET_PPC
	printf("\t-l, --cpu=<0-%d>|<range>|<list>\n", MAX_PROCESSORS-1);
#endif
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
	printf("\t-D, --debug=<debug level>\n");
	printf("\t\t0:error (default) 1:warning 2:notice 3:info 4:debug\n");
	printf("\t-V, --version\n");
	printf("\t-h, --help\n");
	printf("\n");
	printf(" Commands:\n");
	for (i = 0; i < ARRAY_SIZE(actions); i++)
		printf("  %-15s %-27s  %s\n", actions[i].name, actions[i].args, actions[i].desc);
}

/* Parse argument of the form 0-5,7,9-11,15,17 */
static bool parse_list(const char *arg, int max, int *list, int *count)
{
	char str[strlen(arg)+1];
	char *tok, *tmp, *saveptr = NULL;
	int i;

	assert(max < INT_MAX);

	strcpy(str, arg);

	for (i = 0; i < max; i++) {
		list[i] = 0;
	}

	tmp = str;
	while ((tok = strtok_r(tmp, ",", &saveptr)) != NULL) {
		char *a, *b, *endptr, *saveptr2 = NULL;
		unsigned long int from, to;

		a = strtok_r(tok, "-", &saveptr2);
		if (a == NULL) {
			return false;
		} else {
			endptr = NULL;
			from = strtoul(a, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "Invalid value %s\n", a);
				return false;
			}
			if (from >= max) {
				fprintf(stderr, "Value %s larger than max %d\n", a, max-1);
				return false;
			}
		}

		b = strtok_r(NULL, "-", &saveptr2);
		if (b == NULL) {
			to = from;
		} else {
			endptr = NULL;
			to = strtoul(b, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "Invalid value %s\n", b);
				return false;
			}
			if (to >= max) {
				fprintf(stderr, "Value %s larger than max %d\n", b, max-1);
				return false;
			}
		}

		if (from > to) {
			fprintf(stderr, "Invalid range %s-%s\n", a, b);
			return false;
		}

		for (i = from; i <= to; i++)
			list[i] = 1;

		tmp = NULL;
	};

	if (count != NULL) {
		int n = 0;

		for (i = 0; i < max; i++) {
			if (list[i] == 1)
				n++;
		}

		*count = n;
	}

	return true;
}

#ifdef TARGET_PPC
int get_pir(int linux_cpu)
{
	char *filename;
	FILE *file;
	int pir = -1;

	if(asprintf(&filename, "/sys/devices/system/cpu/cpu%i/pir",
		    linux_cpu) < 0)
		return -1;

	file = fopen(filename, "r");
	if (!file) {
		PR_ERROR("Invalid Linux CPU number %" PRIi32 "\n", linux_cpu);
		goto out2;
	}

	if(fscanf(file, "%" PRIx32 "\n", &pir) != 1) {
		PR_ERROR("fscanf() didn't match: %m\n");
		pir = -1;
		goto out1;
	}

out1:
	fclose(file);
out2:
	free(filename);
	return pir;
}

/* Stolen from skiboot */
#define P9_PIR2GCID(pir) (((pir) >> 8) & 0x7f)
#define P9_PIR2COREID(pir) (((pir) >> 2) & 0x3f)
#define P9_PIR2THREADID(pir) ((pir) & 0x3)
#define P8_PIR2GCID(pir) (((pir) >> 7) & 0x3f)
#define P8_PIR2COREID(pir) (((pir) >> 3) & 0xf)
#define P8_PIR2THREADID(pir) ((pir) & 0x7)

void pir_map(int pir, int *chip, int *core, int *thread)
{
	assert(chip && core && thread);

	if (!strncmp(device_node, "p9", 2)) {
		*chip = P9_PIR2GCID(pir);
		*core = P9_PIR2COREID(pir);
		*thread = P9_PIR2THREADID(pir);
	} else if (!strncmp(device_node, "p8", 2)) {
		*chip = P8_PIR2GCID(pir);
		*core = P8_PIR2COREID(pir);
		*thread = P8_PIR2THREADID(pir);
	} else
		assert(0);

}

#define PPC_OPTS "l:"
#else
int get_pir(int linux_cpu) { return -1; }
void pir_map(int pir, int *chip, int *core, int *thread) {}
#define PPC_OPTS
#endif

static bool parse_options(int argc, char *argv[])
{
	int c;
	bool opt_error = false;
	int p_list[MAX_PROCESSORS];
	int c_list[MAX_CHIPS];
	int t_list[MAX_THREADS];
	int l_list[MAX_LINUX_CPUS];
	int p_count = 0, c_count = 0, t_count = 0, l_count = 0;
	int i, j, k;
	struct option long_opts[] = {
		{"all",			no_argument,		NULL,	'a'},
		{"backend",		required_argument,	NULL,	'b'},
		{"chip",		required_argument,	NULL,	'c'},
		{"device",		required_argument,	NULL,	'd'},
		{"help",		no_argument,		NULL,	'h'},
		{"processor",		required_argument,	NULL,	'p'},
		{"slave-address",	required_argument,	NULL,	's'},
		{"thread",		required_argument,	NULL,	't'},
#ifdef TARGET_PPC
		{"cpu",			required_argument,	NULL,	'l'},
#endif
		{"debug",		required_argument,	NULL,	'D'},
		{"version",		no_argument,		NULL,	'V'},
		{NULL,			0,			NULL,     0}
	};
	char *endptr;

	memset(p_list, 0, sizeof(p_list));
	memset(c_list, 0, sizeof(c_list));
	memset(t_list, 0, sizeof(t_list));
	memset(l_list, 0, sizeof(l_list));

	do {
		c = getopt_long(argc, argv, "+ab:c:d:hp:s:t:D:V" PPC_OPTS,
				long_opts, NULL);
		if (c == -1)
			break;

		switch(c) {
		case 'a':
			if (p_count == 0) {
				p_count = MAX_PROCESSORS;
				for (i = 0; i < MAX_PROCESSORS; i++)
					p_list[i] = 1;
			}

			if (c_count == 0) {
				c_count = MAX_CHIPS;
				for (i = 0; i < MAX_CHIPS; i++)
					c_list[i] = 1;
			}

			if (t_count == 0) {
				t_count = MAX_THREADS;
				for (i = 0; i < MAX_THREADS; i++)
					t_list[i] = 1;
			}
			break;

		case 'p':
			if (!parse_list(optarg, MAX_PROCESSORS, p_list, &p_count)) {
				fprintf(stderr, "Failed to parse '-p %s'\n", optarg);
				opt_error = true;
			}
			break;

		case 'c':
			if (!parse_list(optarg, MAX_CHIPS, c_list, &c_count)) {
				fprintf(stderr, "Failed to parse '-c %s'\n", optarg);
				opt_error = true;
			}
			break;

		case 't':
			if (!parse_list(optarg, MAX_THREADS, t_list, &t_count)) {
				fprintf(stderr, "Failed to parse '-t %s'\n", optarg);
				opt_error = true;
			}
			break;

		case 'l':
			if (!parse_list(optarg, MAX_PROCESSORS, l_list, &l_count)) {
				fprintf(stderr, "Failed to parse '-l %s'\n", optarg);
				opt_error = true;
			}
			break;

		case 'b':
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
			} else {
				fprintf(stderr, "Invalid backend '%s'\n", optarg);
				opt_error = true;
			}
			break;

		case 'd':
			device_node = optarg;
			break;

		case 's':
			errno = 0;
			i2c_addr = strtoull(optarg, &endptr, 0);
			opt_error = (errno || *endptr != '\0');
			if (opt_error)
				fprintf(stderr, "Invalid slave address '%s'\n", optarg);
			break;

		case 'D':
			pdbg_set_loglevel(atoi(optarg));
			break;

		case 'V':
			printf("%s (commit %s)\n", PACKAGE_STRING, GIT_SHA1);
			exit(0);
			break;

		case '?':
		case 'h':
			opt_error = true;
			print_usage(basename(argv[0]));
			break;
		}
	} while (c != EOF && !opt_error);

	if (opt_error) {
		return false;
	}

	if ((c_count > 0 || t_count > 0 || p_count > 0) && (l_count > 0)) {
		fprintf(stderr, "Can't mix -l with -p/-c/-t/-a\n");
		return false;
	}

	if ((c_count > 0 || t_count > 0) && p_count == 0) {
		fprintf(stderr, "No processor(s) selected\n");
		fprintf(stderr, "Use -p or -a to select processor(s)\n");
		return false;
	}

	if (t_count > 0 && c_count == 0)  {
		fprintf(stderr, "No chip(s) selected\n");
		fprintf(stderr, "Use -c or -a to select chip(s)\n");
		return false;
	}

	for (i = 0; i < MAX_PROCESSORS; i++) {
		if (p_list[i] == 0)
			continue;

		processorsel[i] = &chipsel[i][0];

		for (j = 0; j < MAX_CHIPS; j++) {
			if (c_list[j] == 0)
				continue;

			chipsel[i][j] = &threadsel[i][j][0];

			for (k = 0; k < MAX_THREADS; k++) {
				if (t_list[k] == 0)
					continue;

				threadsel[i][j][k] = 1;
			}
		}
	}

	if (l_count) {
		int pir = -1, i, chip, core, thread;

		for (i = 0; i < MAX_PROCESSORS; i++) {
			if (l_list[i] == 1) {
				pir = get_pir(i);
				if (pir < 0)
					return true;
				break;
			}
		}
		if (pir < 0)
			return true;

		pir_map(pir, &chip, &core, &thread);

		threadsel[chip][core][thread] = 1;
		chipsel[chip][core] = &threadsel[chip][core][thread];
		processorsel[chip] = &chipsel[chip][core];
	}
	return true;
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

void for_each_target_release(char *class)
{
	struct pdbg_target *target;

	pdbg_for_each_class_target(class, target) {
		if (!target_selected(target))
			continue;

		pdbg_target_release(target);
	}
}

static int target_selection(void)
{
	struct pdbg_target *fsi, *pib, *chip, *thread;

	switch (backend) {
#ifdef TARGET_ARM
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

#endif

#ifdef TARGET_PPC
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
#endif

	case FAKE:
		pdbg_targets_init(&_binary_fake_dtb_o_start);
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
				if (pdbg_parent_index(chip, "pib") != proc_index)
					continue;

				if (chipsel[proc_index][chip_index]) {
					target_select(chip);
					pdbg_for_each_target("thread", chip, thread) {
						int thread_index = pdbg_target_index(thread);
						if (threadsel[proc_index][chip_index][thread_index])
							target_select(thread);
						else
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

static void release_target(struct pdbg_target *target)
{
	struct pdbg_target *child;

	/* !selected targets may get selected in other ways */

	/* Does this target actually exist? */
	if ((pdbg_target_status(target) != PDBG_TARGET_ENABLED) &&
	    (pdbg_target_status(target) != PDBG_TARGET_PENDING_RELEASE))
		return;

	pdbg_for_each_child_target(target, child)
		release_target(child);

	pdbg_target_release(target);
}

static int release(void)
{
	struct pdbg_target_class *target_class;

	for_each_target_class(target_class) {
		struct pdbg_target *target;

		pdbg_for_each_class_target(target_class->name, target)
			release_target(target);
	}

	return 0;
}
OPTCMD_DEFINE_CMD(release, release);

void print_target(struct pdbg_target *target, int level)
{
	int i;
	struct pdbg_target *next;
	enum pdbg_target_status status;

	pdbg_target_probe(target);

	/* Does this target actually exist? */
	status = pdbg_target_status(target);
	if (status != PDBG_TARGET_ENABLED)
		return;

	for (i = 0; i < level; i++)
		printf("    ");

	if (target) {
		char c = 0;

		if (!pdbg_target_class_name(target))
			return;

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

static int probe(void)
{
	struct pdbg_target *target;

	pdbg_for_each_class_target("pib", target)
		print_target(target, 0);

	printf("\nNote that only selected targets will be shown above. If none are shown\n"
			"try adding '-a' to select all targets\n");

	return 1;
}
OPTCMD_DEFINE_CMD(probe, probe);

/*
 * Release handler.
 */
static void atexit_release(void)
{
	release();
}

int main(int argc, char *argv[])
{
	int i, rc = 0;
	void **args, **flags;
	optcmd_cmd_t *cmd;

	backend = default_backend();
	device_node = default_target(backend);

	if (!parse_options(argc, argv))
		return 1;

	if (!backend_is_possible(backend)) {
		fprintf(stderr, "Backend not possible\nUse: ");
		print_backends(stderr);
		return 1;
	}

	if (!target_is_possible(backend, device_node)) {
		fprintf(stderr, "Target %s not possible\n",
			device_node ? device_node : "(none)");
		print_targets(stderr);
		return 1;
	}

	if (optind >= argc) {
		print_usage(basename(argv[0]));
		return 1;
	}

	/* Disable unselected targets */
	if (target_selection())
		return 1;

	atexit(atexit_release);

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (!strcmp(argv[optind], cmds[i]->cmd)) {
			/* Found our command */
			cmd = optcmd_parse(cmds[i], (const char **) &argv[optind + 1],
					   argc - (optind + 1), &args, &flags);
			if (cmd) {
				rc = cmd(args, flags);
				goto found_action;
			} else {
				/* Error parsing arguments so exit return directly */
				return 1;
			}
		}
	}

	/* Process subcommands. Currently only 'htm'.
	 * TODO: Move htm command parsing to optcmd once htm clean-up is complete */
	if (!strcmp(argv[optind], "htm")) {
		rc = run_htm(optind, argc, argv);
		goto found_action;
	}

	PR_ERROR("Unsupported command: %s\n", argv[optind]);
	return 1;

found_action:
	if (rc > 0)
		return 0;

	printf("No valid targets found or specified. Try adding -p/-c/-t options to specify a target.\n");
	printf("Alternatively run '%s -a probe' to get a list of all valid targets\n",
	       basename(argv[0]));
	return 1;
}
