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
#include <assert.h>

#include <ccan/array_size/array_size.h>

#include <libpdbg.h>
#include <device.h>
#include <bitutils.h>

#include "main.h"

#define HTM_ENUM_TO_STRING(e) ((e == HTM_NEST) ? "nhtm" : "chtm")

#define PR_ERROR(x, args...) \
	pdbg_log(PDBG_ERROR, x, ##args)

enum htm_type {
	HTM_CORE,
	HTM_NEST,
};

static inline void print_htm_address(enum htm_type type,
	struct pdbg_target *target)
{
	if (type == HTM_CORE) {
		printf("p%d:", pdbg_parent_index(target, "pib"));
		printf("c%d:", pdbg_parent_index(target, "core"));
	}
	printf("t%d\n", pdbg_target_index(target));
}

static char *get_htm_dump_filename(struct pdbg_target *target)
{
	char *filename;
	int rc;

	rc = asprintf(&filename, "htm-p%d-c%d-t%d.dump",
		      pdbg_parent_index(target, "pib"),
		      pdbg_parent_index(target, "core"),
		      pdbg_target_index(target));
	if (rc == -1)
		return NULL;

	return filename;
}

static int run_start(enum htm_type type)
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("Starting with buffer wrapping HTM@");
		print_htm_address(type, target);
		if (htm_start(target) != 1) {
			printf("Couldn't start HTM@");
			print_htm_address(type, target);
		}
		rc++;
	}

	return rc;
}

static int run_stop(enum htm_type type)
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

static int run_status(enum htm_type type)
{
	struct pdbg_target *target;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		printf("Status HTM@");
		print_htm_address(type, target);
		if (htm_status(target) != 1) {
			printf("Couldn't get HTM@");
			print_htm_address(type, target);
		}
		rc++;
	}

	return rc;
}

static int run_dump(enum htm_type type)
{
	struct pdbg_target *target;
	char *filename;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		filename = get_htm_dump_filename(target);
		if (!filename)
			return 0;

		/* size = 0 will dump everything */
		printf("Dumping HTM@");
		print_htm_address(type, target);
		if (htm_dump(target, filename) != 1) {
			printf("Couldn't dump HTM@");
			print_htm_address(type, target);
		}
		rc++;
		free(filename);
	}

	return rc;
}

static int run_record(enum htm_type type)
{
	struct pdbg_target *target;
	char *filename;
	int rc = 0;

	pdbg_for_each_class_target(HTM_ENUM_TO_STRING(type), target) {
		if (!target_selected(target))
			continue;
		pdbg_target_probe(target);
		if (target_is_disabled(target))
			continue;

		filename = get_htm_dump_filename(target);
		if (!filename)
			return 0;

		/* size = 0 will dump everything */
		printf("Recording till buffer wraps HTM@");
		print_htm_address(type, target);
		if (htm_record(target, filename) != 1) {
			printf("Couldn't record HTM@");
			print_htm_address(type, target);
		}
		rc++;
		free(filename);
	}

	return rc;
}

static struct {
	const char *name;
	const char *args;
	const char *desc;
	int (*fn)(enum htm_type);
} actions[] = {
	{ "start",  "", "Start %s HTM",               &run_start  },
	{ "stop",   "", "Stop %s HTM",                &run_stop   },
	{ "status", "", "Get %s HTM status",          &run_status },
	{ "dump",   "", "Dump %s HTM buffer to file", &run_dump   },
	{ "record", "", "Start, wait & dump %s HTM",  &run_record },
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

static bool is_smt1(struct pdbg_target *target)
{
	/* primary thread */
	if (pdbg_target_index(target) == 0) {
		if (((thread_status(target).active)) &&
		    (thread_status(target).sleep_state == PDBG_THREAD_STATE_RUN))
			return true;
		goto fail;
	}

	/* secondary thread */
	if (thread_status(target).quiesced)
		return true;

fail:
	fprintf(stderr, "Error: core HTM needs to run in SMT1 with no powersave. Try\n");
	fprintf(stderr, "  ppc64_cpu --smt=1\n");
	fprintf(stderr, "  for i in /sys/devices/system/cpu/cpu*/cpuidle/state*/disable;do echo 1 > $i;done\n");
	return false;
}

int run_htm(int optind, int argc, char *argv[])
{
	struct pdbg_target *target, *nhtm;
	enum htm_type type;
	struct pdbg_target *core_target = NULL;
	int i, rc = 0;

	/*
	 * As the index of the last argument is one less than argc, the difference
	 * between optind and argc will always be at least 1. Here optind is pointing
	 * to the 'htm' arg and we need at least 2 more following arguments, eg:
	 * htm <nest/core> <start/stop/etc>
	 * so argc-optind >= 3 to proceed.
	 */
	if (argc - optind < 3) {
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

		/* Check that powersave is off */
		pdbg_for_each_class_target("thread", target) {
			pdbg_target_probe(target);

			if (pdbg_target_status(target) == PDBG_TARGET_NONEXISTENT)
				continue;

			if (!is_smt1(target))
				return 0;
		}

		/* Select the correct chtm target */
		pdbg_for_each_child_target(core_target, target) {
			if (!strcmp(pdbg_target_class_name(target), "chtm")) {
				target_select(target);
				pdbg_target_probe(target);
			}
		}
	}

	if (type == HTM_NEST) {
		pdbg_for_each_class_target("pib", target) {
			if (!target_selected(target))
				continue;

			pdbg_for_each_target("nhtm", target, nhtm)
				target_select(nhtm);
		}
	}

	optind++;
	for (i = 0; i < ARRAY_SIZE(actions); i++) {
		if (strcmp(argv[optind], actions[i].name) == 0) {
			rc = actions[i].fn(type);
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
