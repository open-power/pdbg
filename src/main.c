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

#include "htm.h"
#include "optcmd.h"
#include "progress.h"
#include "pdbgproxy.h"
#include "util.h"
#include "path.h"

#define PR_ERROR(x, args...) \
	pdbg_log(PDBG_ERROR, x, ##args)

#define THREADS_PER_CORE	8

static enum pdbg_backend backend = PDBG_DEFAULT_BACKEND;

static char const *device_node;
static int i2c_addr = 0x50;

#define MAX_PROCESSORS 64
#define MAX_CHIPS 24
#define MAX_THREADS THREADS_PER_CORE

#define MAX_LINUX_CPUS	(MAX_PROCESSORS * MAX_CHIPS * MAX_THREADS)

#define MAX_PATH_ARGS	16

static const char *pathsel[MAX_PATH_ARGS];
static int pathsel_count;

static int probe(void);

/* TODO: We are repeating ourselves here. A little bit more macro magic could
 * easily fix this but I was hesitant to introduce too much magic all at
 * once. */
extern struct optcmd_cmd
	optcmd_getscom, optcmd_putscom,	optcmd_getcfam, optcmd_putcfam,
	optcmd_getgpr, optcmd_putgpr, optcmd_getspr, optcmd_putspr,
	optcmd_getnia, optcmd_putnia, optcmd_getmsr, optcmd_putmsr,
	optcmd_getring, optcmd_start, optcmd_stop, optcmd_step,
	optcmd_threadstatus, optcmd_sreset, optcmd_regs, optcmd_probe,
	optcmd_getmem, optcmd_putmem, optcmd_getmemio, optcmd_putmemio,
	optcmd_getmempba, optcmd_putmempba,
	optcmd_getxer, optcmd_putxer, optcmd_getcr, optcmd_putcr,
	optcmd_gdbserver, optcmd_istep;

static struct optcmd_cmd *cmds[] = {
	&optcmd_getscom, &optcmd_putscom, &optcmd_getcfam, &optcmd_putcfam,
	&optcmd_getgpr, &optcmd_putgpr, &optcmd_getspr, &optcmd_putspr,
	&optcmd_getnia, &optcmd_putnia, &optcmd_getmsr, &optcmd_putmsr,
	&optcmd_getring, &optcmd_start, &optcmd_stop, &optcmd_step,
	&optcmd_threadstatus, &optcmd_sreset, &optcmd_regs, &optcmd_probe,
	&optcmd_getmem, &optcmd_putmem, &optcmd_getmemio, &optcmd_putmemio,
	&optcmd_getmempba, &optcmd_putmempba,
	&optcmd_getxer, &optcmd_putxer, &optcmd_getcr, &optcmd_putcr,
	&optcmd_gdbserver, &optcmd_istep,
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
	{ "getcr",  "", "Get Condition Register (CR)" },
	{ "putcr",  "<value>", "Write Condition Register (CR)" },
	{ "getxer",  "", "Get Fixed Point Exception Register (XER)" },
	{ "putxer",  "<value>", "Write Fixed Point Exception Register (XER)" },
	{ "getring", "<addr> <len>", "Read a ring. Length must be correct" },
	{ "start",   "", "Start thread" },
	{ "step",    "<count>", "Set a thread <count> instructions" },
	{ "stop",    "", "Stop thread" },
	{ "htm", "core|nest start|stop|status|dump|record", "Hardware Trace Macro" },
	{ "probe", "", "" },
	{ "getcfam", "<address>", "Read system cfam" },
	{ "putcfam", "<address> <value> [<mask>]", "Write system cfam" },
	{ "getscom", "<address>", "Read system scom" },
	{ "putscom", "<address> <value> [<mask>]", "Write system scom" },
	{ "getmem",  "<address> <count> [--ci] [--raw]", "Read system memory" },
	{ "getmempba",  "<address> <count> [--ci] [--raw]", "Read system memory" },
	{ "getmemio", "<address> <count> <block size> [--raw]", "Read memory cache inhibited with specified transfer size" },
	{ "putmem",  "<address> [--ci]", "Write to system memory" },
	{ "putmempba",  "<address> [--ci]", "Write to system memory" },
	{ "putmemio", "<address> <block size>", "Write system memory cache inhibited with specified transfer size" },
	{ "threadstatus", "", "Print the status of a thread" },
	{ "sreset",  "", "Reset" },
	{ "regs",  "[--backtrace]", "State (optionally display backtrace)" },
	{ "gdbserver", "", "Start a gdb server" },
	{ "istep", "<major> <minor>|0", "Execute istep on SBE" },
};

static void print_usage(void)
{
	int i;

	printf("Usage: pdbg [options] command ...\n\n");
	printf(" Options:\n");
	printf("\t-p, --processor=<0-%d>|<range>|<list>\n", MAX_PROCESSORS-1);
	printf("\t-c, --chip=<0-%d>|<range>|<list>\n", MAX_CHIPS-1);
	printf("\t-t, --thread=<0-%d>|<range>|<list>\n", MAX_THREADS-1);
#ifdef TARGET_PPC
	printf("\t-l, --cpu=<0-%d>|<range>|<list>\n", MAX_PROCESSORS-1);
#endif
	printf("\t-P, --path=<device tree node spec>\n");
	printf("\t-a, --all\n");
	printf("\t\tRun command on all possible processors/chips/threads (default)\n");
	printf("\t-b, --backend=backend\n");
	printf("\t\tcronus:\tA backend based on cronus server\n");
	printf("\t\tsbefifo:\tA backend using sbefifo kernel driver\n");
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
	printf("\t-S, --shutup\n");
	printf("\t\tShut up those annoying progress bars\n");
	printf("\t-V, --version\n");
	printf("\t-h, --help\n");
	printf("\n");
	printf(" Commands:\n");
	for (i = 0; i < ARRAY_SIZE(actions); i++)
		printf("  %-15s %-27s  %s\n", actions[i].name, actions[i].args, actions[i].desc);
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

static bool pathsel_add(char *format, ...) __attribute__((format (printf, 1, 2)));

static bool pathsel_add(char *format, ...)
{
	va_list ap;
	char path[1024];
	int len;

	va_start(ap, format);

	len = vsnprintf(path, sizeof(path), format, ap);
	if (len > sizeof(path)) {
		va_end(ap);
		return false;
	}

	va_end(ap);

	if (pathsel_count == MAX_PATH_ARGS) {
		fprintf(stderr, "Too many path arguments\n");
		return false;
	}

	pathsel[pathsel_count] = strdup(path);
	assert(pathsel[pathsel_count]);
	pathsel_count++;

	return true;
}

static bool list_to_string(int *list, int max, char *str, size_t len)
{
	char tmp[16];
	int i;

	memset(str, 0, len);

	for (i=0; i<max; i++) {
		if (list[i] == 1) {
			sprintf(tmp, "%d,", i);
			if (strlen(str) + strlen(tmp) + 1 > len) {
				return false;
			}
			strcat(str, tmp);
		}
	}

	return true;
}

static bool parse_options(int argc, char *argv[])
{
	int c;
	bool opt_error = false;
	int p_list[MAX_PROCESSORS];
	int c_list[MAX_CHIPS];
	int t_list[MAX_THREADS];
	int l_list[MAX_LINUX_CPUS];
	int p_count = 0, c_count = 0, t_count = 0, l_count = 0;
	int i;
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
		{"path",		required_argument,	NULL,	'P'},
		{"shutup",		no_argument,		NULL,	'S'},
		{"version",		no_argument,		NULL,	'V'},
		{NULL,			0,			NULL,     0}
	};
	char *endptr;
	char p_str[256], c_str[256], t_str[256];

	memset(p_list, 0, sizeof(p_list));
	memset(c_list, 0, sizeof(c_list));
	memset(t_list, 0, sizeof(t_list));
	memset(l_list, 0, sizeof(l_list));

	do {
		c = getopt_long(argc, argv, "+ab:c:d:hp:s:t:D:P:SV" PPC_OPTS,
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
			if (!parse_list(optarg, MAX_LINUX_CPUS, l_list, &l_count)) {
				fprintf(stderr, "Failed to parse '-l %s'\n", optarg);
				opt_error = true;
			}
			break;

		case 'b':
			if (strcmp(optarg, "fsi") == 0) {
				backend = PDBG_BACKEND_FSI;
			} else if (strcmp(optarg, "i2c") == 0) {
				backend = PDBG_BACKEND_I2C;
			} else if (strcmp(optarg, "kernel") == 0) {
				backend = PDBG_BACKEND_KERNEL;
				/* TODO: use device node to point at a slave
				 * other than the first? */
			} else if (strcmp(optarg, "fake") == 0) {
				backend = PDBG_BACKEND_FAKE;
			} else if (strcmp(optarg, "host") == 0) {
				backend = PDBG_BACKEND_HOST;
			} else if (strcmp(optarg, "cronus") == 0) {
				backend = PDBG_BACKEND_CRONUS;
			} else if (strcmp(optarg, "sbefifo") == 0) {
				backend = PDBG_BACKEND_SBEFIFO;
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

		case 'P':
			if (!pathsel_add("%s", optarg))
				opt_error = true;
			break;

		case 'S':
			progress_shutup();
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
			print_usage();
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

	if (pathsel_count > 0 && l_count > 0) {
		fprintf(stderr, "Can't mix -l with -P\n");
		return false;
	}

	if ((c_count > 0 || t_count > 0 || p_count > 0) && (pathsel_count > 0)) {
		fprintf(stderr, "Can't mix -P with -p/-c/-t/-a\n");
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

	if (p_count) {
		if (!list_to_string(p_list, MAX_PROCESSORS, p_str, sizeof(p_str)))
			return false;
		if (!pathsel_add("fsi[%s]", p_str))
			return false;
		if (!pathsel_add("pib[%s]", p_str))
			return false;
	}

	if (c_count) {
		if (!list_to_string(c_list, MAX_CHIPS, c_str, sizeof(c_str)))
			return false;
		if (!pathsel_add("pib[%s]/core[%s]", p_str, c_str))
			return false;
	}

	if (t_count) {
		if (!list_to_string(t_list, MAX_THREADS, t_str, sizeof(t_str)))
			return false;
		if (!pathsel_add("pib[%s]/core[%s]/thread[%s]", p_str, c_str, t_str))
			return false;
	}

	if (l_count) {
		int pir = -1, i, chip, core, thread;

		if (!device_node)
			return false;

		for (i = 0; i < MAX_LINUX_CPUS; i++) {
			if (l_list[i] == 1) {
				pir = get_pir(i);
				if (pir < 0)
					return true;

				pir_map(pir, &chip, &core, &thread);

				if (!pathsel_add("pib%d", chip))
					return false;
				if (!pathsel_add("pib%d/core%d", chip, core))
					return false;
				if (!pathsel_add("pib%d/core%d/thread%d", chip, core, thread))
					return false;
			}
		}
	}

	return true;
}

static bool child_enabled(struct pdbg_target *target)
{
	struct pdbg_target *child;

	pdbg_for_each_child_target(target, child) {
		if (child_enabled(child))
			return true;

		if (pdbg_target_status(child) == PDBG_TARGET_ENABLED)
			return true;
	}

	return false;
}

static void print_target(struct pdbg_target *target, int level)
{
	int i;
	struct pdbg_target *next;
	enum pdbg_target_status status;
	const char *classname;

	if (level == 0 && !child_enabled(target))
		return;

	/* Does this target actually exist? */
	status = pdbg_target_status(target);
	if (status != PDBG_TARGET_ENABLED)
		return;

	for (i = 0; i < level; i++)
		printf("    ");

	classname = pdbg_target_class_name(target);
	if (classname)
		printf("%s%d: %s", classname, pdbg_target_index(target), pdbg_target_name(target));
	else
		printf("%s:", pdbg_target_dn_name(target));
	if (path_target_selected(target))
		printf(" (*)");
	printf("\n");

	pdbg_for_each_child_target(target, next) {
		print_target(next, level + 1);
	}
}

static int probe(void)
{
	struct pdbg_target *target;

	pdbg_for_each_child_target(pdbg_target_root(), target) {
		print_target(target, 0);
	}

	return 1;
}
OPTCMD_DEFINE_CMD(probe, probe);

/*
 * Release handler.
 */
static void atexit_release(void)
{
	pdbg_target_release(pdbg_target_root());
}

int main(int argc, char *argv[])
{
	int i, rc = 0;
	void **args, **flags;
	optcmd_cmd_t *cmd;
	struct pdbg_target *target;

	if (!parse_options(argc, argv))
		return 1;

	if (optind >= argc) {
		print_usage();
		return 1;
	}

	if (backend)
		if (!pdbg_set_backend(backend, device_node))
			return 1;

	if (!pdbg_targets_init(NULL))
		return 1;

	if (pathsel_count) {
		if (!path_target_parse(pathsel, pathsel_count))
			return 1;
	}

	if (!path_target_present()) {
		printf("No valid targets found or specified. Try adding -p/-c/-t options to specify a target.\n");
		printf("Alternatively run 'pdbg -a probe' to get a list of all valid targets\n");
		return 1;
	}

	/* Probe all selected targets */
	for_each_path_target(target) {
		pdbg_target_probe(target);
	}

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

	return 1;
}
