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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "optcmd.h"

/* Parse a flag of the form "--long-flag=<blah>" or "-F <blah>" */
static int optcmd_parse_flag(const char *argv, struct optcmd_flag *flags,
			     int flag_count, void **flag_results)
{
	int i;
	char *flag, *arg;

	flag = strdup(argv);
	arg = strchr(flag, '=');
	if (arg) {
		*arg = '\0';
		arg++;
	}

	for (i = 0; i < flag_count; i++) {
		if (!strcmp(flag, flags[i].name)) {
			flag_results[i] = flags[i].arg(arg);
			if (!flag_results[i]) {
				printf("Unable to parse argument for %s\n", flag);
				return 1;
			}

			return 0;
		}
	}

	printf("Invalid flag %s specified\n", flag);
	return 1;
}

/* Parse command arguments and flags */
optcmd_cmd_t *optcmd_parse(struct optcmd_cmd *cmd, const char *argv[], int argc,
			   void **arg_results[], void **flag_results[])
{
	int i, arg_count = 0, total_arg_count, total_flag_count;
	struct optcmd_arg *args = cmd->args;
	struct optcmd_flag *flags = cmd->flags;
	void **tmp_arg_results, **tmp_flag_results;

	/* Allocate space for parser results */
	for (total_arg_count = 0; args[total_arg_count].parser && total_arg_count < OPTCMD_MAX_ARGS; total_arg_count++) {}
	for (total_flag_count = 0; flags[total_flag_count].arg && total_flag_count < OPTCMD_MAX_FLAGS; total_flag_count++) {}
	tmp_arg_results = malloc(total_arg_count*sizeof(void *));
	assert(tmp_arg_results);
	tmp_flag_results = calloc(1, total_flag_count*sizeof(void *));
	assert(tmp_flag_results);

	for (i = 0; i < argc; i++, argv++) {
		if (!strncmp(*argv, "--", 2)) {
			if (optcmd_parse_flag(*argv, flags, total_flag_count, tmp_flag_results))
				/* Unable to parse flag */
				return NULL;
		} else {
			if (arg_count >= total_arg_count) {
				printf("Too many arguments passed to %s\n", cmd->cmd);
				return NULL;
			}

			tmp_arg_results[arg_count] = args[arg_count].parser(*argv);
			if (!tmp_arg_results[arg_count]) {
				printf("Unable to parse argument %s\n", *argv);
				return NULL;
			}
			arg_count++;
		}
	}

	for (; arg_count < total_arg_count; arg_count++) {
		/* Process default positional arguments */
		struct optcmd_arg *arg = &args[arg_count];

		if (!arg->def) {
			printf("Not enough arguments passed to %s\n", cmd->cmd);
			return NULL;
		}

		tmp_arg_results[arg_count] = arg->parser(arg->def);
		if (!tmp_arg_results[arg_count]) {
			printf("Programming error - unable to parse default argument %s\n", arg->def);
			return NULL;
		}
	}

	*arg_results = tmp_arg_results;
	*flag_results = tmp_flag_results;

	return cmd->cmdp;
}
