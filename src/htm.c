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



 /*
 * This file does the Hardware Trace Macro command parsing for pdbg
 * the program.
 * It will call into libpdbg backend with a target to either a 'nhtm'
 * or a 'chtm' which will ultimately do the work.
 *
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ccan/array_size/array_size.h>

#include <target.h>
#include <operations.h>

#include "main.h"

#define HTM_DUMP_BASENAME "htm.dump"

#define HTM_ENUM_TO_STRING(e) ((e == HTM_NEST) ? "nhtm" : "chtm")

enum htm_type {
	HTM_CORE,
	HTM_NEST,
};

static inline void print_htm_address(enum htm_type type,
	struct pdbg_target *target)
{
	if (type == HTM_CORE)
		printf("%d#", pdbg_parent_index(target, "core"));
	printf("%d\n", pdbg_target_index(target));
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

static int run_start(enum htm_type type, int optind, int argc, char *argv[])
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("Starting HTM@");
		print_htm_address(type, target);
		if (htm_start(target) != 1) {
			printf("Couldn't start HTM@");
			print_htm_address(type, target);
		}
		rc++;
	}

	return rc;
}

static int run_stop(enum htm_type type, int optind, int argc, char *argv[])
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("Stopping HTM@");
		print_htm_address(type, target);
		if (htm_stop(target) != 1) {
			printf("Couldn't stop HTM@");
			print_htm_address(type, target);
		}
		rc++;
	}

	return rc;
}

static int run_status(enum htm_type type, int optind, int argc, char *argv[])
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("HTM@");
		print_htm_address(type, target);
		if (htm_status(target) != 1) {
			printf("Couldn't get HTM@");
			print_htm_address(type, target);
		}
		rc++;
		printf("\n\n");
	}

	return rc;
}

static int run_reset(enum htm_type type, int optind, int argc, char *argv[])
{
	uint64_t old_base = 0, base, size;
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("Resetting HTM@");
		print_htm_address(type, target);
		if (htm_reset(target, &base, &size) != 1) {
			printf("Couldn't reset HTM@");
			print_htm_address(type, target);
		}
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

static int run_dump(enum htm_type type, int optind, int argc, char *argv[])
{
	struct pdbg_target *target;
	char *filename;
	int rc = 0;

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	/* size = 0 will dump everything */
	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("Dumping HTM@");
		print_htm_address(type, target);
		if (htm_dump(target, 0, filename) != 1) {
			printf("Couldn't dump HTM@");
			print_htm_address(type, target);
		}
		rc++;
	}
	free(filename);

	return rc;
}

static int run_trace(enum htm_type type, int optind, int argc, char *argv[])
{
	int rc;

	rc = run_reset(type, optind, argc, argv);
	if (rc == 0) {
		printf("No HTM units were reset.\n");
		printf("It is unlikely anything will start... trying anyway\n");
	}

	rc = run_start(type, optind, argc, argv);
	if (rc == 0)
		printf("No HTM units were started\n");

	return rc;
}

static int run_analyse(enum htm_type type, int optind, int argc, char *argv[])
{
	int rc;

	rc = run_stop(type, optind, argc, argv);
	if (rc == 0) {
		printf("No HTM units were stopped.\n");
		printf("It is unlikely anything will dump... trying anyway\n");
	}

	rc = run_dump(type, optind, argc, argv);
	if (rc == 0)
		printf("No HTM buffers were dumped to file\n");

	return rc;
}

static struct {
	const char *name;
	const char *args;
	const char *desc;
	int (*fn)(enum htm_type, int, int, char **);
} actions[] = {
	{ "start",  "", "Start %s HTM",               &run_start  },
	{ "stop",   "", "Stop %s HTM",                &run_stop   },
	{ "status", "", "Get %s HTM status",          &run_status },
	{ "reset",  "", "Reset %s HTM",               &run_reset  },
	{ "dump",   "", "Dump %s HTM buffer to file", &run_dump   },
	{ "trace",  "", "Configure and start %s HTM", &run_trace  },
	{ "analyse","", "Stop and dump %s HTM",       &run_analyse},
};

static void print_usage(enum htm_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(actions); i++) {
		printf("%s %s", actions[i].name, actions[i].args);
		printf(actions[i].desc, HTM_ENUM_TO_STRING(type));
		printf("\n");
	}
}

int run_htm(int optind, int argc, char *argv[])
{
	struct pdbg_target *target;
	enum htm_type type;
	struct pdbg_target *core_target = NULL;
	int i, rc = 0;

	if (argc - optind < 2) {
		fprintf(stderr, "Expecting one of 'core' or 'nest' with a command\n");
		return 0;
	}

	optind++;
	if (strcmp(argv[optind], "core") == 0) {
		type = HTM_CORE;
	} else if (strcmp(argv[optind], "nest") == 0) {
		type = HTM_NEST;
	} else {
		fprintf(stderr, "Expecting one of 'core' or 'nest' not %s\n",
			argv[optind]);
		return 0;
	}

	if (type == HTM_CORE) {
		fprintf(stderr, "Warning: Core HTM is currently experimental\n");

		pdbg_for_each_class_target("core", target) {
			if (target_selected(target)) {
				if (!core_target) {
					core_target = target;
				} else {
					fprintf(stderr, "It doesn't make sense to core trace on"
						" multiple cores at once.\n");
					fprintf(stderr, "What you probably want is -p 0 -c x\n");
					return 0;
				}
			}
		}

		if (!core_target) {
			fprintf(stderr, "You haven't selected any HTM Cores\n");
			return 0;
		}

		/*
		 * Check that powersave is off.
		 *
		 * This is as easy as checking that every single
		 * thread is "ACTIVE" and hasn't gone into any sleep
		 * state.
		 *
		 * On P9 it requires checking for THREAD_STATUS_STOP
		 */
		pdbg_for_each_class_target("thread", target) {
			pdbg_target_probe(target);

			if (pdbg_target_status(target) == PDBG_TARGET_NONEXISTENT)
				continue;

			if ((!(thread_status(target) & THREAD_STATUS_ACTIVE)) ||
			    (thread_status(target) & THREAD_STATUS_STOP)) {
				fprintf(stderr, "It appears powersave is on 0x%"  PRIx64 "%p\n", thread_status(target), target);
				fprintf(stderr, "core HTM needs to run with powersave off\n");
				fprintf(stderr, "Hint: put powersave=off on the kernel commandline\n");
				return 0;
			}
		}

		/* Select the correct chtm target */
		pdbg_for_each_child_target(core_target, target) {
			if (!strcmp(pdbg_target_class_name(target), "chtm")) {
				target_select(target);
				pdbg_target_probe(target);
			}
		}
	}

	optind++;
	for (i = 0; i < ARRAY_SIZE(actions); i++) {
		if (strcmp(argv[optind], actions[i].name) == 0) {
			rc = actions[i].fn(type, optind, argc, argv);
			break;
		}
	}

	if (i == ARRAY_SIZE(actions)) {
		PR_ERROR("Unsupported command: %s\n", argv[optind]);
		print_usage(type);
		return 0;
	} else if (rc == 0) {
		fprintf(stderr, "Couldn't run the HTM command.\n");
		fprintf(stderr, "Double check that your kernel has debugfs mounted and the memtrace patches\n");
	}

	return rc;
}

/*
 * Handle the deprecated commands by telling the user what the new
 * command is.
 */
int run_htm_start(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest start\n");

	return 0;
}

int run_htm_stop(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest stop\n");

	return 0;
}

int run_htm_status(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest status\n");

	return 0;
}

int run_htm_reset(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest reset\n");

	return 0;
}

int run_htm_dump(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest dump\n");

	return 0;
}

int run_htm_trace(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest trace\n");

	return 0;
}

int run_htm_analyse(int optind, int argc, char *argv[])
{
	int i;

	fprintf(stderr, "You're running a deprecated command!\n");
	fprintf(stderr, "Please use:\n");
	for (i = 0; i < optind; i++)
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "htm nest analyse\n");

	return 0;
}
