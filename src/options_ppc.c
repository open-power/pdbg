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
#include <stdio.h>
#include <string.h>

#include "main.h"

static const char p8[] = "p8";
static const char p9[] = "p9";

enum backend default_backend(void)
{
	return HOST;
}

bool backend_is_possible(enum backend backend)
{
	/* TODO */
	return backend == HOST;
}

void print_backends(FILE *stream)
{
	fprintf(stream, "HOST");
}

const char *default_target(enum backend backend)
{
	const char *pos = NULL;
	char line[256];
	FILE *cpuinfo;

	cpuinfo = fopen("/proc/cpuinfo", "r");
	if (!cpuinfo)
		return NULL;

	while ((pos = fgets(line, sizeof(line), cpuinfo)))
		if (strncmp(line, "cpu", 3) == 0)
			break;
	fclose(cpuinfo);

	if (!pos) /* Got to EOF without a break */
		return NULL;

	pos = strchr(line, ':');
	if (!pos)
		return NULL;

	if (*(pos + 1) == '\0')
		return NULL;
	pos += 2;

	if (strncmp(pos, "POWER8", 6) == 0)
		return p8;

	if (strncmp(pos, "POWER9", 6) == 0)
		return p9;

	return NULL;
}

void print_targets(FILE *stream)
{
	fprintf(stream, "HOST: p8 p9\n");
}

bool target_is_possible(enum backend backend, const char *target)
{
	const char *def;

	if (!backend_is_possible(backend))
		return false;

	def = default_target(backend);
	if (!def || !target)
		return false;

	return strcmp(def, target) == 0;
}
