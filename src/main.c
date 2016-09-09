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

#include <backend.h>
#include <operations.h>

/* EX/Chiplet GP0 SCOM address */
#define SCOM_EX_GP0	0x10000000

enum command { GETCFAM = 1, PUTCFAM, GETSCOM, PUTSCOM,	\
	       GETMEM, PUTMEM, GETGPR, GETNIA, GETSPR,	\
	       STOPCHIP, STARTCHIP, THREADSTATUS,	\
	       PROBE };

#define MAX_CMD_ARGS 2
enum command cmd = 0;
static int cmd_arg_count = 0;

/* At the moment all commands only take some kind of number */
static uint64_t cmd_args[MAX_CMD_ARGS];

static int processor = 0;
static int chip = 0;
static int thread = 0;

static void print_usage(char *pname)
{
	printf("Usage: %s [options] command ...\n\n", pname);
	printf(" Options:\n");
	printf("\t-p, --processor=processor-id\n");
	printf("\t-c, --chip=chiplet-id\n");
	printf("\t-t, --thread=thread\n");
	printf("\t-h, --help\n");
	printf("\n");
	printf(" Commands:\n");
	printf("\tgetcfam <address>\n");
	printf("\tputcfam <address> <value>\n");
	printf("\tgetscom <address>\n");
	printf("\tputscom <address> <value>\n");
	printf("\tgetmem <address> <count>\n");
	printf("\tgetgpr <gpr>\n");
	printf("\tgetnia\n");
	printf("\tgetspr <spr>\n");
	printf("\tstopchip\n");
	printf("\tstartchip <threads>\n");
	printf("\tthreadstatus\n");
	printf("\tprobe\n");
}

enum command parse_cmd(char *optarg)
{
	if (strcmp(optarg, "getcfam") == 0) {
		cmd = GETCFAM;
		cmd_arg_count = 1;
	} else if (strcmp(optarg, "getscom") == 0) {
		cmd = GETSCOM;
		cmd_arg_count = 1;
	} else if (strcmp(optarg, "putcfam") == 0) {
		cmd = PUTCFAM;
		cmd_arg_count = 2;
	} else if (strcmp(optarg, "putscom") == 0) {
		cmd = PUTSCOM;
		cmd_arg_count = 2;
	} else if (strcmp(optarg, "getmem") == 0) {
		cmd = GETMEM;
		cmd_arg_count = 2;
	} else if (strcmp(optarg, "getgpr") == 0) {
		cmd = GETGPR;
		cmd_arg_count = 1;
	} else if (strcmp(optarg, "getnia") == 0) {
		cmd = GETNIA;
		cmd_arg_count = 0;
	} else if (strcmp(optarg, "getspr") == 0) {
		cmd = GETSPR;
		cmd_arg_count = 1;
	} else if (strcmp(optarg, "stopchip") == 0) {
		cmd = STOPCHIP;
		cmd_arg_count = 0;
	} else if (strcmp(optarg, "startchip") == 0) {
		cmd = STARTCHIP;
		cmd_arg_count = 1;
	} else if (strcmp(optarg, "threadstatus") == 0) {
		cmd = THREADSTATUS;
		cmd_arg_count = 0;
	} else if (strcmp(optarg, "probe") == 0) {
		cmd = PROBE;
		cmd_arg_count = 0;
	}

	return cmd;
}

static bool parse_options(int argc, char *argv[])
{
	int c, oidx = 0, cmd_arg_idx = 0;
	bool opt_error = true;
	struct option long_opts[] = {
		{"processor",	required_argument,	NULL,	'p'},
		{"chip",	required_argument,	NULL,	'c'},
		{"thread",	required_argument,	NULL,	't'},
		{"help",	no_argument,		NULL,	'h'},
	};

	do {
		c = getopt_long(argc, argv, "-p:c:t:h", long_opts, &oidx);
		switch(c) {
		case 1:
			/* Positional argument */
			if (!cmd)
				opt_error = !parse_cmd(optarg);
			else if (cmd_arg_idx >= MAX_CMD_ARGS ||
				 (cmd && cmd_arg_idx >= cmd_arg_count))
				opt_error = true;
			else {
				errno = 0;
				cmd_args[cmd_arg_idx++] = strtoull(optarg, NULL, 0);
				opt_error = errno;
			}
			break;

		case 'p':
			errno = 0;
			processor = strtoull(optarg, NULL, 0);
			opt_error = errno;
			break;
		case 't':
			errno = 0;
			thread = strtoull(optarg, NULL, 0);
			opt_error = errno;
			break;

		case 'c':
			errno = 0;
			chip = strtoull(optarg, NULL, 0);
			opt_error = errno;
			break;

		case 'h':
			opt_error = true;
			break;
		}
	} while (c != EOF && !opt_error);

	opt_error |= cmd_arg_idx < cmd_arg_count;
	if (opt_error)
		print_usage(argv[0]);

	return opt_error;
}

static uint64_t probe(void)
{
	int i, j;
	uint64_t addr, value;

	/* Probe for processors by trying to read all possible
	 * 0xf000f device-id registers. */
	printf("Probing for valid processors...\n");
	for (i = 0; i < 8; i++) {
		backend_set_processor(i);
		if (!getscom(&value, 0xf000f) && value) {
			printf("\tProcessor-ID %d: 0x%llx\n", i, value);
			for (j = 0; j < 0xf; j++) {
				addr = SCOM_EX_GP0 | (j << 24);
				if (!getscom(&value, addr) && value)
					printf("\t\tChiplet-ID %d present\n", j);
			}
		}
	}
}

static uint64_t active_threads;

/*
 * Stop all threads on the chip.
 */
static int stop_chip(void)
{
	int rc;

	if ((rc = ram_stop_chip(chip, &active_threads)))
		PR_ERROR("Error stopping chip\n");

	return rc;
}

static void print_thread_status(void)
{
	int i;
	uint64_t value;

	if (ram_running_threads(chip, &value)) {
		PR_ERROR("Unable to read thread status\n");
		return;
	}

	for (i = 0; i < THREADS_PER_CORE; i++)
			if (value & (1 << (THREADS_PER_CORE - i - 1)))
				printf("Thread %d is RUNNING\n", i);
			else
				printf("Thread %d is STOPPED\n", i);
}

/*
 * Restart previously active threads.
 */
static int start_chip(void)
{
	int rc;

	if ((rc = ram_start_chip(chip, active_threads)))
		PR_ERROR("Error starting chip\n");

	return rc;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	uint32_t u32_value = 0;
	uint64_t value = 0;
	uint8_t *buf;

	if (parse_options(argc, argv))
		return 1;

	if (backend_init(processor)) {
		PR_ERROR("Unable to initialise backend\n");
		return 1;
	}

	switch(cmd) {
	case GETCFAM:
		if (getcfam(&u32_value, cmd_args[0]))
			PR_ERROR("Error writing register\n");
		else
			printf("0x%llx: 0x%08x\n", cmd_args[0], u32_value);
		break;
	case GETSCOM:
		if (getscom(&value, cmd_args[0]))
			PR_ERROR("Error reading register\n");
		else
			printf("0x%llx: 0x%016llx\n", cmd_args[0], value);
		break;
	case PUTCFAM:
		if (putcfam(cmd_args[1], cmd_args[0]))
			PR_ERROR("Error writing register\n");
		else
			printf("0x%llx: 0x%08llx\n", cmd_args[0], cmd_args[1]);
		break;
	case PUTSCOM:
		if (putscom(cmd_args[1], cmd_args[0]))
			PR_ERROR("Error reading register\n");
		else
			printf("0x%llx: 0x%016llx\n", cmd_args[0], cmd_args[1]);
		break;
	case GETMEM:
		buf = malloc(cmd_args[1]);
		if (!adu_getmem(cmd_args[0], buf, cmd_args[1], 8))
			write(STDOUT_FILENO, buf, cmd_args[1]);
		else
			PR_ERROR("Unable to read memory.\n");

		free(buf);
		break;
	case GETGPR:
		if (stop_chip())
			break;

		if (ram_getgpr(chip, thread, cmd_args[0], &value))
			PR_ERROR("Error reading register\n");
		else
			printf("r%lld = 0x%016llx\n", cmd_args[0], value);

		start_chip();
		break;
	case GETNIA:
		if (stop_chip())
			break;

		if (ram_getnia(chip, thread, &value))
			PR_ERROR("Error reading register\n");
		else
			printf("pc = 0x%016llx\n", value);

		start_chip();
		break;
	case GETSPR:
		if (stop_chip())
			break;

		if (ram_getspr(chip, thread, cmd_args[0], &value))
			PR_ERROR("Error reading register\n");
		else
			printf("spr%lld = 0x%016llx\n", cmd_args[0], value);

		start_chip();
		break;
	case STOPCHIP:
		stop_chip();
		printf("Previously active thread mask: 0x%02llx\n", active_threads);
		print_thread_status();
		break;
	case STARTCHIP:
		active_threads = cmd_args[0];
		start_chip();
		print_thread_status();
		break;
	case THREADSTATUS:
		print_thread_status();
		break;
	case PROBE:
		probe();
		break;
	}

	backend_destroy();

	return rc;
}
